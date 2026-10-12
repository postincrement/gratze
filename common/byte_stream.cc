#include "byte_stream.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <spawn.h>

#include <chrono>
#include <cctype>
#include <string>
#include <vector>

#if defined(GRZ_HAVE_OPENSSL)
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

extern char ** environ;

using namespace std;

namespace {

constexpr int kConnectMs = 5000;
constexpr int kWriteMs = 2000;
constexpr int kMaxUrl = 1024;
constexpr size_t kInMax = 64 * 1024;
constexpr size_t kHeaderMax = 8192;

void IgnoreSigPipe()
{
  static bool done = false;
  if (done)
    return;
  done = true;
  // A dropped peer must not kill the emulator. Sockets also set
  // SO_NOSIGPIPE / MSG_NOSIGNAL; this covers the ssh pipes.
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sa, nullptr);
}

void SetCloexec(int fd)
{
  int flags = fcntl(fd, F_GETFD);
  if (flags >= 0)
    fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

void SetNonblock(int fd)
{
  int flags = fcntl(fd, F_GETFL);
  if (flags >= 0)
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void SetNoSigPipe(int fd)
{
#ifdef SO_NOSIGPIPE
  int one = 1;
  setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#else
  (void)fd;
#endif
}

bool WaitFd(int fd, short events, int timeoutMs)
{
  if (fd < 0)
    return false;
  pollfd pfd;
  pfd.fd = fd;
  pfd.events = events;
  pfd.revents = 0;
  for (;;) {
    int rc = poll(&pfd, 1, timeoutMs);
    if (rc < 0 && errno == EINTR)
      continue;
    if (rc <= 0)
      return false;
    return (pfd.revents & (events | POLLERR | POLLHUP | POLLNVAL)) != 0;
  }
}

string Trim(string s)
{
  while (!s.empty() && (s.back() == '\0' || isspace((unsigned char)s.back())))
    s.pop_back();
  size_t i = 0;
  while (i < s.size() && (s[i] == '\0' || isspace((unsigned char)s[i])))
    ++i;
  if (i > 0)
    s.erase(0, i);
  return s;
}

// ssh://user:secret@host becomes ssh://user@host. The secret is never
// stored or passed to ssh.
string Redact(const string & url)
{
  auto scheme = url.find("://");
  if (scheme == string::npos)
    return url;
  auto at = url.find('@', scheme + 3);
  if (at == string::npos)
    return url;
  auto colon = url.find(':', scheme + 3);
  if (colon == string::npos || colon > at)
    return url;
  return url.substr(0, colon) + url.substr(at);
}

string Lower(string s)
{
  for (char & c : s)
    c = (char)tolower((unsigned char)c);
  return s;
}

bool ParseInt(const string & text, int minValue, int maxValue, int & out)
{
  if (text.empty() || text.size() > 6)
    return false;
  char * end = nullptr;
  long v = strtol(text.c_str(), &end, 10);
  if (end == text.c_str() || *end != '\0')
    return false;
  if (v < minValue || v > maxValue)
    return false;
  out = (int)v;
  return true;
}

string QueryValue(const string & query, const string & key)
{
  size_t i = 0;
  while (i < query.size()) {
    size_t amp = query.find('&', i);
    if (amp == string::npos)
      amp = query.size();
    string part = query.substr(i, amp - i);
    auto eq = part.find('=');
    string k = eq == string::npos ? part : part.substr(0, eq);
    string v = eq == string::npos ? string() : part.substr(eq + 1);
    if (Lower(k) == key)
      return v;
    i = amp + 1;
  }
  return {};
}

struct Parsed {
  string scheme;
  string user;
  string host;
  int port = -1;
  string path;
  int baud = 9600;
  bool pty = false;
};

bool ParseUrl(const string & raw, Parsed & out, ByteStream::Error & error)
{
  string text = Trim(raw);
  if (text.empty() || text.size() > kMaxUrl) {
    error = ByteStream::Error::BadUrl;
    return false;
  }

  auto sep = text.find("://");
  if (sep == string::npos || sep == 0) {
    error = ByteStream::Error::BadUrl;
    return false;
  }
  out.scheme = Lower(text.substr(0, sep));
  string rest = text.substr(sep + 3);

  string query;
  auto qpos = rest.find('?');
  if (qpos != string::npos) {
    query = rest.substr(qpos + 1);
    rest.resize(qpos);
  }

  if (out.scheme == "serial") {
    if (rest.empty()) {
      error = ByteStream::Error::BadUrl;
      return false;
    }
    out.path = rest;
    string baud = QueryValue(query, "baud");
    if (!baud.empty() && !ParseInt(baud, 1, 4000000, out.baud)) {
      error = ByteStream::Error::BadUrl;
      return false;
    }
    return true;
  }

  if (out.scheme != "tcp" && out.scheme != "http" && out.scheme != "https" && out.scheme != "ssh") {
    error = ByteStream::Error::Unsupported;
    return false;
  }

  auto slash = rest.find('/');
  string hostport = slash == string::npos ? rest : rest.substr(0, slash);
  out.path = slash == string::npos ? string() : rest.substr(slash);
  if (!query.empty()) {
    if (out.path.empty())
      out.path = "/";
    out.path += "?" + query;
  }

  auto at = hostport.rfind('@');
  if (at != string::npos) {
    out.user = hostport.substr(0, at);
    auto colon = out.user.find(':');
    if (colon != string::npos)
      out.user.resize(colon);
    hostport = hostport.substr(at + 1);
  }

  if (!hostport.empty() && hostport[0] == '[') {
    auto br = hostport.find(']');
    if (br == string::npos) {
      error = ByteStream::Error::BadUrl;
      return false;
    }
    out.host = hostport.substr(1, br - 1);
    if (br + 1 < hostport.size()) {
      if (hostport[br + 1] != ':') {
        error = ByteStream::Error::BadUrl;
        return false;
      }
      if (!ParseInt(hostport.substr(br + 2), 1, 65535, out.port)) {
        error = ByteStream::Error::BadUrl;
        return false;
      }
    }
  }
  else {
    auto colon = hostport.rfind(':');
    if (colon != string::npos) {
      out.host = hostport.substr(0, colon);
      if (!ParseInt(hostport.substr(colon + 1), 1, 65535, out.port)) {
        error = ByteStream::Error::BadUrl;
        return false;
      }
    }
    else {
      out.host = hostport;
    }
  }

  if (out.host.empty()) {
    error = ByteStream::Error::BadUrl;
    return false;
  }

  if (out.scheme == "tcp") {
    if (out.port < 0) {
      error = ByteStream::Error::BadUrl;
      return false;
    }
    return true;
  }
  if (out.scheme == "http") {
    if (out.port < 0)
      out.port = 80;
    if (out.path.empty())
      out.path = "/";
    return true;
  }
  if (out.scheme == "https") {
    if (out.port < 0)
      out.port = 443;
    if (out.path.empty())
      out.path = "/";
    return true;
  }

  if (out.port < 0)
    out.port = 22;
  string pty = QueryValue(query, "pty");
  out.pty = pty == "1" || Lower(pty) == "yes" || Lower(pty) == "true";
  // The HTTP path logic above may have stuffed "?pty=1" into path. The
  // query was already consumed; pty does not belong on an ssh path.
  auto pathQ = out.path.find('?');
  if (pathQ != string::npos)
    out.path.resize(pathQ);
  return true;
}

struct Conn {
  virtual ~Conn() = default;
  // >0 bytes, 0 end, -1 nothing ready, -2 failed.
  virtual int Read(uint8_t * buf, int n) = 0;
  virtual int Write(const uint8_t * buf, int n) = 0;
  virtual bool WaitWrite(int timeoutMs) = 0;
  virtual void Close() = 0;
};

int WriteAll(Conn & conn, const uint8_t * data, int n, int timeoutMs)
{
  auto deadline = chrono::steady_clock::now() + chrono::milliseconds(timeoutMs);
  int off = 0;
  while (off < n) {
    int rc = conn.Write(data + off, n - off);
    if (rc > 0) {
      off += rc;
      continue;
    }
    if (rc != -1)
      return -1;
    auto left = chrono::duration_cast<chrono::milliseconds>(
        deadline - chrono::steady_clock::now()).count();
    if (left <= 0 || !conn.WaitWrite((int)left))
      return -1;
  }
  return off;
}

struct TcpConn : Conn {
  int m_fd = -1;

  explicit TcpConn(int fd)
    : m_fd(fd)
  {}

  ~TcpConn() override
  {
    Close();
  }

  int Read(uint8_t * buf, int n) override
  {
    if (m_fd < 0)
      return -2;
    // read/write so the same conn can sit on a socket or a serial port.
    ssize_t rc = ::read(m_fd, buf, (size_t)n);
    if (rc > 0)
      return (int)rc;
    if (rc == 0)
      return 0;
    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
      return -1;
    return -2;
  }

  int Write(const uint8_t * buf, int n) override
  {
    if (m_fd < 0)
      return -2;
    ssize_t rc = ::write(m_fd, buf, (size_t)n);
    if (rc > 0)
      return (int)rc;
    if (rc < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
      return -1;
    return -2;
  }

  bool WaitWrite(int timeoutMs) override
  {
    return WaitFd(m_fd, POLLOUT, timeoutMs);
  }

  void Close() override
  {
    if (m_fd < 0)
      return;
    ::shutdown(m_fd, SHUT_RDWR);
    ::close(m_fd);
    m_fd = -1;
  }
};

#if defined(GRZ_HAVE_OPENSSL)

SSL_CTX * ClientCtx()
{
  static SSL_CTX * ctx = nullptr;
  if (ctx != nullptr)
    return ctx;
  ctx = SSL_CTX_new(TLS_client_method());
  if (ctx == nullptr)
    return nullptr;
  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
  SSL_CTX_set_default_verify_paths(ctx);
  SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
  return ctx;
}

struct TlsConn : Conn {
  int m_fd = -1;
  SSL * m_ssl = nullptr;
  bool m_wantRead = false;

  TlsConn(int fd, SSL * ssl)
    : m_fd(fd)
    , m_ssl(ssl)
  {}

  ~TlsConn() override
  {
    Close();
  }

  int Read(uint8_t * buf, int n) override
  {
    if (m_ssl == nullptr)
      return -2;
    int rc = SSL_read(m_ssl, buf, n);
    if (rc > 0)
      return rc;
    int e = SSL_get_error(m_ssl, rc);
    if (e == SSL_ERROR_WANT_READ || e == SSL_ERROR_WANT_WRITE)
      return -1;
    if (e == SSL_ERROR_ZERO_RETURN)
      return 0;
    if (e == SSL_ERROR_SYSCALL && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
      return -1;
    return -2;
  }

  int Write(const uint8_t * buf, int n) override
  {
    if (m_ssl == nullptr)
      return -2;
    int rc = SSL_write(m_ssl, buf, n);
    if (rc > 0)
      return rc;
    int e = SSL_get_error(m_ssl, rc);
    if (e == SSL_ERROR_WANT_READ) {
      m_wantRead = true;
      return -1;
    }
    if (e == SSL_ERROR_WANT_WRITE) {
      m_wantRead = false;
      return -1;
    }
    if (e == SSL_ERROR_SYSCALL && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
      return -1;
    return -2;
  }

  bool WaitWrite(int timeoutMs) override
  {
    return WaitFd(m_fd, m_wantRead ? POLLIN : POLLOUT, timeoutMs);
  }

  void Close() override
  {
    if (m_ssl != nullptr) {
      SSL_free(m_ssl);
      m_ssl = nullptr;
    }
    if (m_fd >= 0) {
      ::shutdown(m_fd, SHUT_RDWR);
      ::close(m_fd);
      m_fd = -1;
    }
  }
};

bool TlsHandshake(int fd, SSL * ssl)
{
  auto deadline = chrono::steady_clock::now() + chrono::milliseconds(kConnectMs);
  for (;;) {
    int rc = SSL_connect(ssl);
    if (rc == 1)
      return true;
    int e = SSL_get_error(ssl, rc);
    short ev = 0;
    if (e == SSL_ERROR_WANT_READ)
      ev = POLLIN;
    else if (e == SSL_ERROR_WANT_WRITE)
      ev = POLLOUT;
    else
      return false;
    auto left = chrono::duration_cast<chrono::milliseconds>(
        deadline - chrono::steady_clock::now()).count();
    if (left <= 0 || !WaitFd(fd, ev, (int)left))
      return false;
  }
}

#endif // GRZ_HAVE_OPENSSL

struct SshConn : Conn {
  int m_in = -1;
  int m_out = -1;
  int m_err = -1;
  pid_t m_pid = -1;
  bool m_failed = false;

  ~SshConn() override
  {
    Close();
  }

  void DrainErr()
  {
    if (m_err < 0)
      return;
    uint8_t junk[256];
    for (;;) {
      ssize_t n = ::read(m_err, junk, sizeof(junk));
      if (n < 0 && errno == EINTR)
        continue;
      if (n <= 0)
        break;
    }
  }

  void Reap(bool block)
  {
    if (m_pid < 0)
      return;
    int status = 0;
    pid_t got = ::waitpid(m_pid, &status, block ? 0 : WNOHANG);
    if (got != m_pid)
      return;
    m_pid = -1;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
      m_failed = true;
  }

  int Read(uint8_t * buf, int n) override
  {
    DrainErr();
    if (m_out < 0)
      return m_failed ? -2 : 0;
    ssize_t rc = ::read(m_out, buf, (size_t)n);
    if (rc > 0)
      return (int)rc;
    if (rc < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
      return -1;
    if (rc < 0)
      return -2;
    Reap(false);
    return m_failed ? -2 : 0;
  }

  int Write(const uint8_t * buf, int n) override
  {
    if (m_in < 0)
      return -2;
    ssize_t rc = ::write(m_in, buf, (size_t)n);
    if (rc > 0)
      return (int)rc;
    if (rc < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
      return -1;
    return -2;
  }

  bool WaitWrite(int timeoutMs) override
  {
    return WaitFd(m_in, POLLOUT, timeoutMs);
  }

  void Close() override
  {
    pid_t pid = m_pid;
    if (m_in >= 0)
      ::close(m_in);
    if (m_out >= 0)
      ::close(m_out);
    if (m_err >= 0)
      ::close(m_err);
    m_in = -1;
    m_out = -1;
    m_err = -1;
    m_pid = -1;
    if (pid < 0)
      return;
    ::kill(pid, SIGTERM);
    for (int i = 0; i < 20; ++i) {
      int status = 0;
      if (::waitpid(pid, &status, WNOHANG) == pid)
        return;
      if (i == 10)
        ::kill(pid, SIGKILL);
      pollfd nap;
      nap.fd = -1;
      nap.events = 0;
      nap.revents = 0;
      ::poll(&nap, 1, 20);
    }
    int status = 0;
    ::waitpid(pid, &status, 0);
  }
};

int ConnectTcp(const string & host, int port)
{
  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV;

  addrinfo * result = nullptr;
  string service = to_string(port);
  if (getaddrinfo(host.c_str(), service.c_str(), &hints, &result) != 0)
    return -1;

  int fd = -1;
  for (addrinfo * ai = result; ai != nullptr; ai = ai->ai_next) {
    int sock = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock < 0)
      continue;
    SetCloexec(sock);
    SetNoSigPipe(sock);
    int one = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    SetNonblock(sock);

    int rc = ::connect(sock, ai->ai_addr, ai->ai_addrlen);
    if (rc < 0 && errno != EINPROGRESS) {
      ::close(sock);
      continue;
    }
    if (rc < 0 && !WaitFd(sock, POLLOUT, kConnectMs)) {
      ::close(sock);
      continue;
    }
    int so = 0;
    socklen_t len = sizeof(so);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &so, &len) < 0 || so != 0) {
      ::close(sock);
      continue;
    }
    fd = sock;
    break;
  }
  freeaddrinfo(result);
  return fd;
}

string HttpRequest(const Parsed & url)
{
  string host = url.host;
  if (url.host.find(':') != string::npos)
    host = "[" + url.host + "]";
  bool defaultPort = (url.scheme == "http" && url.port == 80)
                  || (url.scheme == "https" && url.port == 443);
  string request = "GET " + url.path + " HTTP/1.0\r\nHost: " + host;
  if (!defaultPort)
    request += ":" + to_string(url.port);
  request += "\r\nUser-Agent: gratze\r\nAccept: */*\r\nConnection: close\r\n\r\n";
  return request;
}

speed_t BaudSpeed(int baud, bool & ok)
{
  ok = true;
  switch (baud) {
    case 300: return B300;
    case 1200: return B1200;
    case 2400: return B2400;
    case 4800: return B4800;
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
#ifdef B230400
    case 230400: return B230400;
#endif
    default:
      ok = false;
      return B9600;
  }
}

void SetError(ByteStream::Error * error, ByteStream::Error value)
{
  if (error != nullptr)
    *error = value;
}

struct Slot {
  bool used = false;
  bool open = false;
  bool readOnly = false;
  bool http = false;
  bool headersDone = true;
  bool eof = false;
  ByteStream::Error error = ByteStream::Error::None;
  string name;
  string header;
  vector<uint8_t> in;
  size_t inAt = 0;
  unique_ptr<Conn> conn;

  bool HasByte() const
  {
    return inAt < in.size();
  }

  void Compact()
  {
    if (inAt == 0)
      return;
    if (inAt >= in.size() || inAt > 2048) {
      in.erase(in.begin(), in.begin() + (ptrdiff_t)inAt);
      inAt = 0;
    }
  }

  void Accept(const uint8_t * data, int n)
  {
    if (!http || headersDone) {
      in.insert(in.end(), data, data + n);
      return;
    }
    header.append((const char *)data, (size_t)n);
    size_t pos = header.find("\r\n\r\n");
    size_t sep = 4;
    if (pos == string::npos) {
      pos = header.find("\n\n");
      sep = 2;
    }
    if (pos == string::npos) {
      if (header.size() > kHeaderMax) {
        error = ByteStream::Error::Io;
        eof = true;
        header.clear();
      }
      return;
    }
    string rest = header.substr(pos + sep);
    header.clear();
    headersDone = true;
    if (!rest.empty())
      in.insert(in.end(), rest.begin(), rest.end());
  }

  void Refresh()
  {
    // EOF can arrive in the same read as the last bytes. open stays true
    // until those bytes are taken, including on a later call that does
    // not read again.
    if (eof && !headersDone && error == ByteStream::Error::None)
      error = ByteStream::Error::Io;
    if (eof && !HasByte())
      open = false;
    Compact();
  }

  void Pump()
  {
    if (conn && !eof) {
      uint8_t buf[1024];
      while ((in.size() - inAt) < kInMax) {
        int n = conn->Read(buf, (int)sizeof(buf));
        if (n == -1)
          break;
        if (n == 0) {
          eof = true;
          break;
        }
        if (n < 0) {
          error = ByteStream::Error::Io;
          eof = true;
          break;
        }
        Accept(buf, n);
      }
    }
    Refresh();
  }

  ByteStream::Error WriteByte(uint8_t byte)
  {
    if (!used || !conn)
      return ByteStream::Error::NoConnection;
    if (!open)
      return error != ByteStream::Error::None ? error : ByteStream::Error::Closed;
    if (readOnly)
      return ByteStream::Error::ReadOnly;
    if (WriteAll(*conn, &byte, 1, kWriteMs) != 1) {
      error = ByteStream::Error::Io;
      return error;
    }
    return ByteStream::Error::None;
  }

  void Shutdown()
  {
    if (conn) {
      conn->Close();
      conn.reset();
    }
    open = false;
    eof = true;
  }
};

ByteStream::Error OpenSocket(Slot & slot, const Parsed & url, bool tls)
{
  int fd = ConnectTcp(url.host, url.port);
  if (fd < 0)
    return ByteStream::Error::Connect;

  if (tls) {
#if defined(GRZ_HAVE_OPENSSL)
    SSL_CTX * ctx = ClientCtx();
    if (ctx == nullptr) {
      ::close(fd);
      return ByteStream::Error::Connect;
    }
    SSL * ssl = SSL_new(ctx);
    if (ssl == nullptr) {
      ::close(fd);
      return ByteStream::Error::Connect;
    }
    BIO * bio = BIO_new_socket(fd, BIO_NOCLOSE);
    if (bio == nullptr) {
      SSL_free(ssl);
      ::close(fd);
      return ByteStream::Error::Connect;
    }
    SSL_set_bio(ssl, bio, bio);
    SSL_set_tlsext_host_name(ssl, url.host.c_str());
    SSL_set1_host(ssl, url.host.c_str());
    SSL_set_mode(ssl, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    SSL_clear_mode(ssl, SSL_MODE_AUTO_RETRY);
    ERR_clear_error();
    if (!TlsHandshake(fd, ssl)) {
      SSL_free(ssl);
      ::close(fd);
      return ByteStream::Error::Connect;
    }
    slot.conn.reset(new TlsConn(fd, ssl));
#else
    ::close(fd);
    return ByteStream::Error::Unsupported;
#endif
  }
  else {
    slot.conn.reset(new TcpConn(fd));
  }

  if (slot.http) {
    string request = HttpRequest(url);
    if (WriteAll(*slot.conn, (const uint8_t *)request.data(), (int)request.size(), kWriteMs) < 0) {
      slot.Shutdown();
      return ByteStream::Error::Io;
    }
  }
  return ByteStream::Error::None;
}

bool PipePair(int fds[2])
{
  if (pipe(fds) < 0) {
    fds[0] = -1;
    fds[1] = -1;
    return false;
  }
  SetCloexec(fds[0]);
  SetCloexec(fds[1]);
  return true;
}

ByteStream::Error OpenSsh(Slot & slot, const Parsed & url)
{
  int inPipe[2] = { -1, -1 };
  int outPipe[2] = { -1, -1 };
  int errPipe[2] = { -1, -1 };
  if (!PipePair(inPipe) || !PipePair(outPipe) || !PipePair(errPipe)) {
    for (int fd : { inPipe[0], inPipe[1], outPipe[0], outPipe[1], errPipe[0], errPipe[1] }) {
      if (fd >= 0)
        ::close(fd);
    }
    return ByteStream::Error::Connect;
  }

  string port = to_string(url.port);
  string dest = url.user.empty() ? url.host : url.user + "@" + url.host;
  vector<string> args = {
    "ssh",
    url.pty ? "-tt" : "-T",
    "-o", "BatchMode=yes",
    "-o", "StrictHostKeyChecking=accept-new",
    "-o", "ConnectTimeout=8",
    "-p", port,
    dest
  };
  vector<char *> argv;
  argv.reserve(args.size() + 1);
  for (string & arg : args)
    argv.push_back(arg.data());
  argv.push_back(nullptr);

  posix_spawn_file_actions_t actions;
  posix_spawn_file_actions_init(&actions);
  posix_spawn_file_actions_adddup2(&actions, inPipe[0], STDIN_FILENO);
  posix_spawn_file_actions_adddup2(&actions, outPipe[1], STDOUT_FILENO);
  posix_spawn_file_actions_adddup2(&actions, errPipe[1], STDERR_FILENO);
  posix_spawn_file_actions_addclose(&actions, inPipe[0]);
  posix_spawn_file_actions_addclose(&actions, inPipe[1]);
  posix_spawn_file_actions_addclose(&actions, outPipe[0]);
  posix_spawn_file_actions_addclose(&actions, outPipe[1]);
  posix_spawn_file_actions_addclose(&actions, errPipe[0]);
  posix_spawn_file_actions_addclose(&actions, errPipe[1]);

  pid_t pid = -1;
  int spawned = posix_spawnp(&pid, "ssh", &actions, nullptr, argv.data(), environ);
  posix_spawn_file_actions_destroy(&actions);
  ::close(inPipe[0]);
  ::close(outPipe[1]);
  ::close(errPipe[1]);

  if (spawned != 0) {
    ::close(inPipe[1]);
    ::close(outPipe[0]);
    ::close(errPipe[0]);
    return ByteStream::Error::Connect;
  }

  auto ssh = unique_ptr<SshConn>(new SshConn);
  ssh->m_in = inPipe[1];
  ssh->m_out = outPipe[0];
  ssh->m_err = errPipe[0];
  ssh->m_pid = pid;
  SetNonblock(ssh->m_in);
  SetNonblock(ssh->m_out);
  SetNonblock(ssh->m_err);
  slot.conn = std::move(ssh);
  return ByteStream::Error::None;
}

ByteStream::Error OpenSerial(Slot & slot, const Parsed & url)
{
  bool ok = false;
  speed_t speed = BaudSpeed(url.baud, ok);
  if (!ok)
    return ByteStream::Error::BadUrl;

  int fd = ::open(url.path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
  if (fd < 0)
    return ByteStream::Error::Connect;
  SetNoSigPipe(fd);

  termios tio;
  if (tcgetattr(fd, &tio) < 0) {
    ::close(fd);
    return ByteStream::Error::Connect;
  }
  cfmakeraw(&tio);
  cfsetispeed(&tio, speed);
  cfsetospeed(&tio, speed);
  tio.c_cflag |= (tcflag_t)(CLOCAL | CREAD | CS8);
  tio.c_cflag &= (tcflag_t)~(PARENB | CSTOPB);
  tio.c_cc[VMIN] = 0;
  tio.c_cc[VTIME] = 0;
  if (tcsetattr(fd, TCSANOW, &tio) < 0) {
    ::close(fd);
    return ByteStream::Error::Connect;
  }

  slot.conn.reset(new TcpConn(fd));
  return ByteStream::Error::None;
}

} // namespace

struct ByteStream::Impl {
  Slot m_slots[kMaxConnections];

  Slot * Find(int id)
  {
    if (id < 1 || id > kMaxConnections)
      return nullptr;
    Slot & slot = m_slots[id - 1];
    if (!slot.used)
      return nullptr;
    return &slot;
  }

  int Alloc()
  {
    for (int i = 0; i < kMaxConnections; ++i) {
      if (!m_slots[i].used)
        return i;
    }
    return -1;
  }
};

ByteStream::ByteStream()
  : m_impl(new Impl)
{
  IgnoreSigPipe();
}

ByteStream::~ByteStream() = default;

int ByteStream::Open(const string & url, Error * error)
{
  Parsed parsed;
  Error perr = Error::None;
  if (!ParseUrl(url, parsed, perr)) {
    SetError(error, perr);
    return 0;
  }

  int index = m_impl->Alloc();
  if (index < 0) {
    SetError(error, Error::Full);
    return 0;
  }

  Slot fresh;
  fresh.name = Redact(Trim(url));
  fresh.http = parsed.scheme == "http" || parsed.scheme == "https";
  fresh.readOnly = fresh.http;
  fresh.headersDone = !fresh.http;

  Error oerr = Error::None;
  if (parsed.scheme == "tcp")
    oerr = OpenSocket(fresh, parsed, false);
  else if (parsed.scheme == "http")
    oerr = OpenSocket(fresh, parsed, false);
  else if (parsed.scheme == "https")
    oerr = OpenSocket(fresh, parsed, true);
  else if (parsed.scheme == "ssh")
    oerr = OpenSsh(fresh, parsed);
  else if (parsed.scheme == "serial")
    oerr = OpenSerial(fresh, parsed);
  else
    oerr = Error::Unsupported;

  if (oerr != Error::None) {
    fresh.Shutdown();
    SetError(error, oerr);
    return 0;
  }

  fresh.used = true;
  fresh.open = true;
  fresh.error = Error::None;
  m_impl->m_slots[index] = std::move(fresh);
  SetError(error, Error::None);
  return index + 1;
}

ByteStream::Error ByteStream::Close(int id)
{
  Slot * slot = m_impl->Find(id);
  if (slot == nullptr)
    return Error::NoConnection;
  slot->Shutdown();
  *slot = Slot();
  return Error::None;
}

ByteStream::Error ByteStream::Status(int id, State & out)
{
  out = State();
  Slot * slot = m_impl->Find(id);
  if (slot == nullptr) {
    out.error = Error::NoConnection;
    return Error::NoConnection;
  }
  slot->Pump();
  out.name = slot->name;
  out.open = slot->open;
  out.error = slot->error;
  return Error::None;
}

bool ByteStream::Const(int id, Error * error)
{
  Slot * slot = m_impl->Find(id);
  if (slot == nullptr) {
    SetError(error, Error::NoConnection);
    return false;
  }
  slot->Pump();
  if (slot->HasByte()) {
    SetError(error, Error::None);
    return true;
  }
  if (slot->open) {
    SetError(error, Error::None);
    return false;
  }
  SetError(error, slot->error != Error::None ? slot->error : Error::Closed);
  return false;
}

int ByteStream::Conin(int id, Error * error)
{
  if (!Const(id, error))
    return -1;
  Slot * slot = m_impl->Find(id);
  if (slot == nullptr || !slot->HasByte()) {
    SetError(error, Error::Closed);
    return -1;
  }
  uint8_t byte = slot->in[slot->inAt++];
  slot->Refresh();
  SetError(error, Error::None);
  return byte;
}

ByteStream::Error ByteStream::Conout(int id, uint8_t byte)
{
  Slot * slot = m_impl->Find(id);
  if (slot == nullptr)
    return Error::NoConnection;
  return slot->WriteByte(byte);
}
