#ifndef STARNET_SERVER_H_
#define STARNET_SERVER_H_

#include <stdint.h>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "starnet.h"

// Station side of the link. It has no ready, strobe, or direction state.
// A later program can construct it with a different image and workspace
// and register further commands, including ones past the current enum.
class StarnetServer
{
  public:
    StarnetServer(std::string bootImagePath, std::string workspacePath);

    void Reset();

    // Bytes that follow the 7-byte header for this code. Unknown codes
    // contribute no payload, so the checksum byte stays the next one.
    uint16_t PayloadLength(uint8_t code, uint16_t parm0, uint16_t parm1, uint16_t parm2) const;

    // The only way a client asks the station to do something.
    std::vector<StarnetFrame> Handle(const StarnetRequest & request);

    using PayloadFn = std::function<uint16_t (uint16_t, uint16_t, uint16_t)>;
    using Handler = std::function<std::vector<StarnetFrame> (const StarnetRequest &)>;

    void Register(uint8_t code, uint16_t payloadBytes, Handler handler);
    void Register(uint8_t code, PayloadFn payload, Handler handler);

  protected:
    struct Command {
      PayloadFn m_payload;
      Handler m_handler;
    };

    std::vector<StarnetFrame> OnBoot(const StarnetRequest & request);
    std::vector<StarnetFrame> OnLogin(const StarnetRequest & request);
    std::vector<StarnetFrame> OnLogout(const StarnetRequest & request);
    std::vector<StarnetFrame> OnRead(const StarnetRequest & request);
    std::vector<StarnetFrame> OnWrite(const StarnetRequest & request);
    std::vector<StarnetFrame> OnUnimplemented(const StarnetRequest & request);

    bool LoadBootImage();
    bool OpenWorkspace();
    uint32_t RecordNumber(const StarnetRequest & request) const;
    static StarnetFrame Reply(Starnet::ResponseType code,
                              uint16_t parm0 = 0,
                              uint16_t parm1 = 0,
                              uint16_t parm2 = 0,
                              const uint8_t * data = nullptr,
                              uint16_t len = 0);
    static const char * CommandName(uint8_t code);

    std::string m_bootImagePath;
    std::string m_workspacePath;
    std::vector<uint8_t> m_bootImage;
    bool m_loggedIn = false;
    std::map<uint8_t, Command> m_commands;
};

#endif // STARNET_SERVER_H_
