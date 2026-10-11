#ifndef STARNET_UDP_PORT_H_
#define STARNET_UDP_PORT_H_

#include <stdint.h>
#include <vector>

#include <netinet/in.h>

#include "starnet_port.h"

// Bytes the PIO protocol never sees. The station id selects one of the
// 16 slots. The sequence lets a retransmit reuse the cached replies.
// replyIndex and replyCount put a multi-frame answer back in order.
struct StarnetShim
{
  uint8_t station = 0;
  uint8_t sequence = 0;
  uint8_t replyIndex = 0;
  uint8_t replyCount = 0;
};

class UdpPort : public StarnetPort
{
  public:
    UdpPort();
    ~UdpPort();

    UdpPort(const UdpPort &) = delete;
    UdpPort & operator=(const UdpPort &) = delete;

    // Server. Binds 127.0.0.1 and the given port.
    bool Bind(uint16_t port);
    // Client. Sends to 127.0.0.1. Nothing has to be listening.
    bool Connect(uint16_t port);

    // Zero waits forever. A positive value is how long Read waits.
    void SetReadTimeoutMs(int ms);

    // One datagram. The shim is stripped before the wire bytes are copied.
    // Negative means timeout or error. Zero is a datagram with no wire bytes.
    int Read(uint8_t * dst, int n) override;
    int Write(const uint8_t * src, int n) override;
    bool WaitForReceiver() override;

    void SetShim(const StarnetShim & shim);
    const StarnetShim & Shim() const;
    void SetPeer(const sockaddr_in & peer);
    const sockaddr_in & Peer() const;

    // Send one request and collect its replies in index order. The same
    // sequence is retransmitted until the set arrives. `station` becomes
    // the assigned id, or kAssignStation when the server rejects it.
    bool Exchange(uint8_t & station,
                  const uint8_t * request,
                  int requestLen,
                  std::vector<std::vector<uint8_t>> & replies);

  private:
    int m_fd = -1;
    bool m_connected = false;
    int m_timeoutMs = 0;
    uint8_t m_sequence = 0;
    StarnetShim m_shim;
    sockaddr_in m_peer{};
};

#endif // STARNET_UDP_PORT_H_
