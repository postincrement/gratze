#ifndef BYTE_STREAM_H_
#define BYTE_STREAM_H_

#include <stdint.h>
#include <memory>
#include <string>

// Byte streams shared by the emulated BIOS trap interface and the starnet
// server. One object owns every open connection.
//
// Open accepts a URL:
//   tcp://host:port                  raw bidirectional TCP
//   http://host[:port][/path]        HTTP/1.0 GET; Conin reads the body
//   https://host[:port][/path]       same, over TLS (needs OpenSSL)
//   ssh://[user@]host[:port][?pty=1] remote shell via the system ssh
//   serial://device[?baud=N]         host serial port, 8N1, default 9600
//
// http and https are read-only. ssh with ?pty=1 asks ssh for a tty;
// without it the channel is a raw byte pipe. A password in an ssh or
// http userinfo is ignored and is not kept on the connection.
//
// Connection ids are 1..kMaxConnections. Zero is never a connection.
// The same function numbers are the BIOS trap codes and the starnet
// ByteStream secondary code.

class ByteStream
{
  public:
    static constexpr int kMaxConnections = 16;

    // Guest status block written by the BIOS trap at DE. 128 bytes:
    //   +0 open     0x00 closed, 0xFF open
    //   +1 error    Error code below
    //   +2 nameLen  0..kStatusNameMax
    //   +3 name     nameLen bytes, then a NUL
    static constexpr int kStatusBytes = 128;
    static constexpr int kStatusNameMax = 124;

    enum class Fn : uint8_t {
      Open   = 0xe0,
      Close  = 0xe1,
      Status = 0xe2,
      Const  = 0xe3,
      Conin  = 0xe4,
      Conout = 0xe5
    };

    enum class Error : uint8_t {
      None         = 0,
      BadUrl       = 1,
      Unsupported  = 2,
      Connect      = 3,
      Closed       = 4,
      NoConnection = 5,
      Full         = 6,
      Io           = 7,
      ReadOnly     = 8
    };

    struct State {
      std::string name;
      bool open = false;
      Error error = Error::None;
    };

    ByteStream();
    ~ByteStream();

    ByteStream(const ByteStream &) = delete;
    ByteStream & operator=(const ByteStream &) = delete;

    // Id of the new connection, or 0. On failure *error is set when non-null.
    int Open(const std::string & url, Error * error);

    Error Close(int id);

    // None when id is open or has ended. NoConnection when id is unknown.
    // The connection's own error is state.error; a clean end is
    // open == false and error == None.
    Error Status(int id, State & state);

    // True when an input byte is waiting. Does not consume it.
    // *error is None while the stream is open, Closed at a clean end
    // once the buffer is empty, or the connection error after a failure.
    bool Const(int id, Error * error);

    // 0..255, or -1 when nothing is waiting. A waiting 0xFF is returned
    // as 255; -1 is the only "no byte" result.
    int Conin(int id, Error * error);

    Error Conout(int id, uint8_t byte);

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

static_assert((int)ByteStream::Fn::Open == 0xe0, "byte stream codes start at 0xe0");
static_assert((int)ByteStream::Fn::Conout == 0xe5, "byte stream codes end at 0xe5");

#endif // BYTE_STREAM_H_
