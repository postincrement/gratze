#ifndef STARNET_SERVER_H_
#define STARNET_SERVER_H_

#include <stdint.h>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "starnet.h"

class CpmDrive;
class CpmDriveSet;

// Station side of the link. It has no ready, strobe, or direction state.
// The drive set is shared by every workstation slot. Login stays per slot.
class StarnetServer
{
  public:
    StarnetServer(std::string bootImagePath, CpmDriveSet * drives);

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

    // Slave image placed at C000. m_length is how many bytes of the
    // image the client stores. After those packets, one more download
    // packet writes the DPB table at C000 + m_dpbOffset. The next drive
    // is 15 bytes later. The server's last frame is the entry jump and
    // is not stored.
    //
    // Default slave is 64kbcpm.slv: network BIOS (READ/WRITE via E042),
    // 128K banked (port 50), DPB at D706, large ALV at DD6E. Length must
    // stop at DF00 — the file is zero-padded through DFFF, and loading
    // that tail wipes the BN ROM IM2 page (I=DF, vector at DF4E).
    // 56kbcpm.slv remains available via --image for the older map.
    struct BootImage {
      const char * m_filename;
      uint16_t m_dpbOffset;
      uint16_t m_length;
    };

    static constexpr BootImage kBootImages[] = {
      { "64kbcpm.slv", 0x1706, 0xdf00 - 0xc000 },
      { "56kbcpm.slv", 0x1648, 60 * 128 },
    };

    std::vector<StarnetFrame> OnBoot(const StarnetRequest & request);
    std::vector<StarnetFrame> OnLogin(const StarnetRequest & request);
    std::vector<StarnetFrame> OnLogout(const StarnetRequest & request);
    std::vector<StarnetFrame> OnRead(const StarnetRequest & request);
    std::vector<StarnetFrame> OnWrite(const StarnetRequest & request);
    std::vector<StarnetFrame> OnUnimplemented(const StarnetRequest & request);

    bool LoadBootImage();
    static const BootImage * BootImageFor(const std::string & path);
    static const BootImage * DefaultBootImage();
    void FillDpbs(uint8_t * dest);
    void LoginProvidedDrives();
    // Bytes of ALV the loaded slave reserves for this DPH slot.
    int AlvBytes(int drive) const;
    bool PrepareDrive(int drive);
    // Reserved tracks (OFF*SPT records) are the SYSGEN image. Serve them
    // from the slave that was downloaded on cold boot.
    bool ReadReservedRecord(const CpmDrive & drive, uint32_t record, uint8_t * dest) const;
    CpmDrive * DriveFor(const StarnetRequest & request);
    int DriveNumber(const StarnetRequest & request) const;
    bool DriveLoggedIn(int number) const;
    bool DriveReadOnly(int number) const;
    static StarnetFrame Reply(Starnet::ResponseType code,
                              uint16_t parm0 = 0,
                              uint16_t parm1 = 0,
                              uint16_t parm2 = 0,
                              const uint8_t * data = nullptr,
                              uint16_t len = 0);
    static const char * CommandName(uint8_t code);

    // The slave images name six drives in the DPH, fifteen bytes apart.
    static const int kDpbSlots = 6;
    // Matches CpmDriveSet::kDrives without pulling that header in here.
    static const int kLoginDrives = 16;

    // Non-empty when --image forces a slave file; otherwise 64kbcpm.slv.
    std::string m_bootImageOverride;
    std::string m_bootImagePath;
    CpmDriveSet * m_drives = nullptr;
    std::vector<uint8_t> m_bootImage;
    // Per-drive login for this workstation slot. Provided drives are
    // logged in as read-only on boot.
    bool m_driveLoggedIn[kLoginDrives] = {};
    bool m_driveReadOnly[kLoginDrives] = {};
    std::map<uint8_t, Command> m_commands;
};

#endif // STARNET_SERVER_H_
