#include "udp_port.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>

#include "starnet.h"

namespace {

const int kShimBytes = 4;
const int kAttempts = 8;
const int kAttemptMs = 100;
const int kSocketBytes = 256 * 1024;

void Enlarge(int fd)
{
  int bytes = kSocketBytes;
  setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bytes, sizeof(bytes));
  setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bytes, sizeof(bytes));
}

void StoreTimeout(int fd, int ms)
{
  timeval tv{};
  if (ms > 0) {
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
  }
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

} // namespace

UdpPort::UdpPort() = default;

UdpPort::~UdpPort()
{
  if (m_fd >= 0)
    close(m_fd);
}

bool UdpPort::Bind(uint16_t port)
{
  m_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (m_fd < 0)
    return false;
  int reuse = 1;
  setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  Enlarge(m_fd);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (bind(m_fd, (sockaddr *)&addr, sizeof(addr)) != 0) {
    close(m_fd);
    m_fd = -1;
    return false;
  }
  SetReadTimeoutMs(0);
  return true;
}

bool UdpPort::Connect(uint16_t port)
{
  m_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (m_fd < 0)
    return false;
  Enlarge(m_fd);
  m_peer.sin_family = AF_INET;
  m_peer.sin_port = htons(port);
  m_peer.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (connect(m_fd, (sockaddr *)&m_peer, sizeof(m_peer)) != 0) {
    close(m_fd);
    m_fd = -1;
    return false;
  }
  m_connected = true;
  SetReadTimeoutMs(0);
  return true;
}

void UdpPort::SetReadTimeoutMs(int ms)
{
  m_timeoutMs = ms;
  if (m_fd >= 0)
    StoreTimeout(m_fd, ms);
}

int UdpPort::Read(uint8_t * dst, int n)
{
  if (m_fd < 0 || n < 0)
    return -1;
  uint8_t buf[4 + 512];
  for (;;) {
    sockaddr_in from{};
    socklen_t fromLen = sizeof(from);
    ssize_t got = recvfrom(m_fd, buf, sizeof(buf), 0, (sockaddr *)&from, &fromLen);
    if (got < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if (got < kShimBytes)
      return -1;
    m_peer = from;
    m_shim.station = buf[0];
    m_shim.sequence = buf[1];
    m_shim.replyIndex = buf[2];
    m_shim.replyCount = buf[3];
    int payload = (int)got - kShimBytes;
    if (payload > n)
      payload = n;
    if (payload > 0 && dst != nullptr)
      memcpy(dst, buf + kShimBytes, (size_t)payload);
    return payload;
  }
}

int UdpPort::Write(const uint8_t * src, int n)
{
  if (m_fd < 0 || n < 0 || n > 512)
    return -1;
  uint8_t buf[4 + 512];
  buf[0] = m_shim.station;
  buf[1] = m_shim.sequence;
  buf[2] = m_shim.replyIndex;
  buf[3] = m_shim.replyCount;
  if (n > 0 && src != nullptr)
    memcpy(buf + kShimBytes, src, (size_t)n);
  ssize_t sent;
  if (m_connected)
    sent = send(m_fd, buf, (size_t)(kShimBytes + n), 0);
  else
    sent = sendto(m_fd, buf, (size_t)(kShimBytes + n), 0,
                  (sockaddr *)&m_peer, sizeof(m_peer));
  if (sent != kShimBytes + n)
    return -1;
  return n;
}

bool UdpPort::WaitForReceiver()
{
  return true;
}

void UdpPort::SetShim(const StarnetShim & shim)
{
  m_shim = shim;
}

const StarnetShim & UdpPort::Shim() const
{
  return m_shim;
}

void UdpPort::SetPeer(const sockaddr_in & peer)
{
  m_peer = peer;
}

const sockaddr_in & UdpPort::Peer() const
{
  return m_peer;
}

bool UdpPort::Exchange(uint8_t & station,
                       const uint8_t * request,
                       int requestLen,
                       std::vector<std::vector<uint8_t>> & replies)
{
  replies.clear();
  if (m_fd < 0 || request == nullptr || requestLen <= 0)
    return false;

  uint8_t sequence = m_sequence++;
  std::vector<std::vector<uint8_t>> got;
  int expect = -1;

  for (int attempt = 0; attempt < kAttempts; ++attempt) {
    m_shim.station = station;
    m_shim.sequence = sequence;
    m_shim.replyIndex = 0;
    m_shim.replyCount = 1;
    if (Write(request, requestLen) < 0)
      return false;

    SetReadTimeoutMs(kAttemptMs);
    for (;;) {
      uint8_t buf[512];
      int n = Read(buf, (int)sizeof(buf));
      if (n < 0)
        break;
      if (m_shim.sequence != sequence)
        continue;
      if (m_shim.station == Starnet::kAssignStation) {
        station = Starnet::kAssignStation;
        SetReadTimeoutMs(0);
        return false;
      }
      if (station == Starnet::kAssignStation)
        station = m_shim.station;
      else if (m_shim.station != station)
        continue;
      if (m_shim.replyCount == 0 || m_shim.replyIndex >= m_shim.replyCount)
        continue;
      if (expect < 0) {
        expect = m_shim.replyCount;
        got.assign((size_t)expect, {});
      }
      else if (m_shim.replyCount != expect)
        continue;
      if (got[(size_t)m_shim.replyIndex].empty() && n > 0)
        got[(size_t)m_shim.replyIndex].assign(buf, buf + n);

      bool complete = true;
      for (const std::vector<uint8_t> & reply : got) {
        if (reply.empty())
          complete = false;
      }
      if (complete) {
        replies = std::move(got);
        SetReadTimeoutMs(0);
        return true;
      }
    }
  }

  SetReadTimeoutMs(0);
  return false;
}
