#ifndef STARNET_SESSION_H_
#define STARNET_SESSION_H_

#include "starnet_port.h"
#include "starnet_server.h"

// Owns the Starnet wire format and calls Handle. It does not know whether
// the port is a datagram or a PIO.
class StarnetSession
{
  public:
    // Read one request, check the checksum, call Handle, and write each
    // reply after WaitForReceiver. A bad checksum is one 0xFF frame and
    // does not call Handle.
    static bool Serve(StarnetServer & server, StarnetPort & port);

  private:
    static std::vector<uint8_t> WireReply(const StarnetFrame & frame);
};

#endif // STARNET_SESSION_H_
