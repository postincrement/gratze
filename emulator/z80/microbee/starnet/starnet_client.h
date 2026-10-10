#ifndef STARNET_CLIENT_H_
#define STARNET_CLIENT_H_

#include <stddef.h>
#include <stdint.h>
#include <deque>
#include <vector>

#include "starnet.h"
#include "starnet_server.h"

// The slave's peer on the wire. Bytes in, bytes out, and the server it
// calls when a frame is complete. It does not open images or track login.
class StarnetClient
{
  public:
    explicit StarnetClient(StarnetServer & server);

    void Reset();

    // One byte the slave transmitted. A leading 0x00 and the length byte
    // that the BN block sender clocks out before the frame are skipped.
    void OnReceive(uint8_t byte);

    // Push a whole frame, or several, the way an external device would.
    void Push(const uint8_t * bytes, size_t len);

    bool HasTx() const;
    // Bytes in the next response frame, not including the length the ROM
    // reads once before that frame. Zero when nothing is queued.
    uint16_t TakeFrameSize();
    uint8_t OnSend();

  protected:
    void AcceptHeaderByte(uint8_t byte);
    void FinishFrame(uint8_t checksum);
    void Queue(const StarnetFrame & frame);
    void Queue(const std::vector<StarnetFrame> & frames);

    enum class RxState {
      Idle,
      SkipLength,
      Header,
      Payload,
      Checksum
    };

    StarnetServer & m_server;
    RxState m_state = RxState::Idle;
    uint8_t m_header[Starnet::kHeaderBytes];
    int m_got = 0;
    std::vector<uint8_t> m_payload;
    uint16_t m_payloadNeed = 0;
    std::vector<uint8_t> m_tx;
    size_t m_txPos = 0;
    std::deque<uint16_t> m_frameSizes;
};

#endif // STARNET_CLIENT_H_
