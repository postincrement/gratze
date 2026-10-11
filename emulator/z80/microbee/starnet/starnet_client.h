#ifndef STARNET_CLIENT_H_
#define STARNET_CLIENT_H_

#include <stddef.h>
#include <stdint.h>
#include <deque>
#include <memory>
#include <vector>

#include "starnet.h"

class UdpPort;

// Bytes between the Microbee PIO and the UDP shim. It keeps the ROM length
// byte and does not call the station. A reset clears this framer only.
class StarnetClient
{
  public:
    StarnetClient();
    ~StarnetClient();

    StarnetClient(const StarnetClient &) = delete;
    StarnetClient & operator=(const StarnetClient &) = delete;

    // Fixed workstation id, 0 through 15. The default asks to be assigned.
    void SetStation(uint8_t station);

    void Reset();

    // One byte the slave transmitted. EDDC clocks 0x00, then the length,
    // then the frame. The length is how many frame bytes follow.
    void OnReceive(uint8_t byte);

    void Push(const uint8_t * bytes, size_t len);

    bool HasTx() const;
    // Bytes in the next response frame, not including the length the ROM
    // reads once before that frame. Zero when nothing is queued.
    uint16_t TakeFrameSize();
    uint8_t OnSend();

  private:
    void Deliver();
    void QueueBytes(const uint8_t * bytes, uint16_t size);

    enum class RxState {
      Idle,
      Length,
      Body
    };

    std::unique_ptr<UdpPort> m_port;
    uint8_t m_station = Starnet::kAssignStation;
    RxState m_state = RxState::Idle;
    int m_bodyLeft = 0;
    std::vector<uint8_t> m_wire;
    std::vector<uint8_t> m_tx;
    size_t m_txPos = 0;
    std::deque<uint16_t> m_frameSizes;
};

#endif // STARNET_CLIENT_H_
