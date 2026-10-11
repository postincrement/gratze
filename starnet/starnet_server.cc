#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include <unistd.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include "cpm_drive.h"
#include "starnet_server.h"

using namespace std;

namespace {

// Relative path under the emulator tree for a named slave image.
const char * kBootImageDir = "z80/microbee/starnet/";
// Both slaves are CP/M systems: CCP at C000, BDOS at C800, BIOS at D600.
const uint16_t kBootAddress = 0xc000;
const uint16_t kBiosAddress = 0xd600;

string ExecutableDir()
{
  char buf[4096];
#if defined(__APPLE__)
  uint32_t size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) != 0)
    return {};
#elif defined(__linux__)
  ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n <= 0)
    return {};
  buf[n] = '\0';
#else
  return {};
#endif
  char real[4096];
  if (realpath(buf, real) == nullptr)
    return {};
  string path(real);
  auto slash = path.find_last_of('/');
  if (slash == string::npos)
    return {};
  path.resize(slash);
  return path;
}

// The slave image lives next to this source. Accept either the emulator
// directory or the repository root as the working directory, and also look
// next to the executable when the program is started from somewhere else.
string FindBootImage(const char * filename)
{
  string relative = string(kBootImageDir) + filename;
  vector<string> candidates;
  candidates.push_back(string("./") + relative);
  candidates.push_back(string("./emulator/") + relative);

  string dir = ExecutableDir();
  for (int i = 0; i < 6 && !dir.empty(); ++i) {
    candidates.push_back(dir + "/" + relative);
    candidates.push_back(dir + "/emulator/" + relative);
    auto slash = dir.find_last_of('/');
    if (slash == string::npos)
      break;
    dir.resize(slash);
  }

  for (const auto & candidate : candidates) {
    if (access(candidate.c_str(), R_OK) == 0)
      return candidate;
  }
  return {};
}

}

const StarnetServer::BootImage * StarnetServer::BootImageFor(const string & path)
{
  auto slash = path.find_last_of('/');
  string name = slash == string::npos ? path : path.substr(slash + 1);
  for (const auto & image : kBootImages) {
    if (name.size() != strlen(image.m_filename))
      continue;
    bool same = true;
    for (size_t i = 0; i < name.size(); ++i) {
      if (tolower((unsigned char)name[i]) != tolower((unsigned char)image.m_filename[i])) {
        same = false;
        break;
      }
    }
    if (same)
      return &image;
  }
  return nullptr;
}

const StarnetServer::BootImage * StarnetServer::DefaultBootImage()
{
  return &kBootImages[0];
}

StarnetServer::StarnetServer(string bootImagePath, CpmDriveSet * drives)
  : m_bootImageOverride(std::move(bootImagePath))
  , m_drives(drives)
{
  auto fixed = [](uint16_t n) {
    return [n](uint16_t, uint16_t, uint16_t) { return n; };
  };
  auto boot = [this](const StarnetRequest & request) { return OnBoot(request); };
  auto login = [this](const StarnetRequest & request) { return OnLogin(request); };
  auto logout = [this](const StarnetRequest & request) { return OnLogout(request); };
  auto read = [this](const StarnetRequest & request) { return OnRead(request); };
  auto write = [this](const StarnetRequest & request) { return OnWrite(request); };
  auto later = [this](const StarnetRequest & request) { return OnUnimplemented(request); };

  using Req = Starnet::RequestType;
  Register((uint8_t)Req::ColdBoot, fixed(0), boot);
  Register((uint8_t)Req::WarmBoot, fixed(0), boot);
  Register((uint8_t)Req::ReadRecord, fixed(0), read);
  Register((uint8_t)Req::WriteRecord, fixed(Starnet::kRecordBytes), write);
  Register((uint8_t)Req::LogIn, fixed(0), login);
  Register((uint8_t)Req::LogOut, fixed(0), logout);

  // The BN BIOS sends 128 bytes with Print. The others carry a header only.
  // Each one is registered so a later handler can replace it without a new framer.
  Register((uint8_t)Req::Print, fixed(Starnet::kRecordBytes), later);
  Register((uint8_t)Req::ChangePassword, fixed(0), later);
  Register((uint8_t)Req::RequestWritePerm, fixed(0), later);
  Register((uint8_t)Req::RelinquishWritePerm, fixed(0), later);
  Register((uint8_t)Req::GetDiskInfo, fixed(0), later);
  Register((uint8_t)Req::GetPermissionData, fixed(0), later);
  Register((uint8_t)Req::GetWorkspaceInfo, fixed(0), later);
  Register((uint8_t)Req::GetConfigDataAndPrintSize, fixed(0), later);
  Register((uint8_t)Req::SendConfigData, fixed(0), later);
  Register((uint8_t)Req::PrintSpoolBuffer, fixed(0), later);
  Register((uint8_t)Req::SendUserCommand, fixed(0), later);
  Register((uint8_t)Req::GetUserCommand, fixed(0), later);
  Register((uint8_t)Req::ClearPrintBuffer, fixed(0), later);
}

void StarnetServer::Reset()
{
  memset(m_driveLoggedIn, 0, sizeof(m_driveLoggedIn));
  memset(m_driveReadOnly, 0, sizeof(m_driveReadOnly));
}

void StarnetServer::Register(uint8_t code, uint16_t payloadBytes, Handler handler)
{
  Register(code, [payloadBytes](uint16_t, uint16_t, uint16_t) { return payloadBytes; }, std::move(handler));
}

void StarnetServer::Register(uint8_t code, PayloadFn payload, Handler handler)
{
  m_commands[code] = Command{std::move(payload), std::move(handler)};
}

uint16_t StarnetServer::PayloadLength(uint8_t code, uint16_t parm0, uint16_t parm1, uint16_t parm2) const
{
  auto it = m_commands.find(code);
  if (it == m_commands.end())
    return 0;
  return it->second.m_payload(parm0, parm1, parm2);
}

std::vector<StarnetFrame> StarnetServer::Handle(const StarnetRequest & request)
{
  auto it = m_commands.find(request.m_code);
  vector<StarnetFrame> frames;
  if (it == m_commands.end())
    frames = { Reply(Starnet::ResponseType::RequestOutOfRange) };
  else
    frames = it->second.m_handler(request);

  ios::fmtflags saved = cerr.flags();
  cerr << "starnet: rx " << CommandName(request.m_code)
       << " (" << dec << (int)request.m_code << ")"
       << hex << setfill('0')
       << " parm " << setw(4) << request.m_parm0
       << " " << setw(4) << request.m_parm1
       << " " << setw(4) << request.m_parm2
       << dec << " payload " << request.m_payload.size()
       << " ->";
  if (frames.empty())
    cerr << " (no reply)";
  else {
    cerr << hex << setfill('0') << " " << setw(2) << (int)frames[0].m_code;
    if (frames.size() > 1)
      cerr << dec << " x" << frames.size();
  }
  cerr << endl;
  cerr.flags(saved);
  return frames;
}

StarnetFrame StarnetServer::Reply(Starnet::ResponseType code,
                                  uint16_t parm0,
                                  uint16_t parm1,
                                  uint16_t parm2,
                                  const uint8_t * data,
                                  uint16_t len)
{
  StarnetFrame frame;
  frame.m_code = (uint8_t)code;
  frame.m_parm0 = parm0;
  frame.m_parm1 = parm1;
  frame.m_parm2 = parm2;
  if (data != nullptr && len != 0)
    frame.m_payload.assign(data, data + len);
  return frame;
}

const char * StarnetServer::CommandName(uint8_t code)
{
  switch ((Starnet::RequestType)code) {
    case Starnet::RequestType::ColdBoot: return "ColdBoot";
    case Starnet::RequestType::WarmBoot: return "WarmBoot";
    case Starnet::RequestType::ReadRecord: return "ReadRecord";
    case Starnet::RequestType::WriteRecord: return "WriteRecord";
    case Starnet::RequestType::LogIn: return "LogIn";
    case Starnet::RequestType::LogOut: return "LogOut";
    case Starnet::RequestType::Print: return "Print";
    case Starnet::RequestType::ChangePassword: return "ChangePassword";
    case Starnet::RequestType::RequestWritePerm: return "RequestWritePerm";
    case Starnet::RequestType::RelinquishWritePerm: return "RelinquishWritePerm";
    case Starnet::RequestType::GetDiskInfo: return "GetDiskInfo";
    case Starnet::RequestType::GetPermissionData: return "GetPermissionData";
    case Starnet::RequestType::GetWorkspaceInfo: return "GetWorkspaceInfo";
    case Starnet::RequestType::GetConfigDataAndPrintSize: return "GetConfigDataAndPrintSize";
    case Starnet::RequestType::SendConfigData: return "SendConfigData";
    case Starnet::RequestType::PrintSpoolBuffer: return "PrintSpoolBuffer";
    case Starnet::RequestType::SendUserCommand: return "SendUserCommand";
    case Starnet::RequestType::GetUserCommand: return "GetUserCommand";
    case Starnet::RequestType::ClearPrintBuffer: return "ClearPrintBuffer";
  }
  return "unknown";
}

bool StarnetServer::LoadBootImage()
{
  string path = m_bootImageOverride;
  const BootImage * selected = nullptr;
  if (path.empty()) {
    selected = DefaultBootImage();
    path = FindBootImage(selected->m_filename);
  }
  else {
    selected = BootImageFor(path);
  }

  if (!m_bootImage.empty() && path == m_bootImagePath)
    return true;

  m_bootImage.clear();
  m_bootImagePath.clear();

  ifstream file;
  if (!path.empty())
    file.open(path, ios::binary);
  streamoff length = -1;
  if (file) {
    file.seekg(0, ios::end);
    length = file.tellg();
    file.seekg(0, ios::beg);
  }
  if (!file || length < 0) {
    const char * name = selected ? selected->m_filename : "boot image";
    cerr << "starnet: could not read '" << (path.empty() ? name : path.c_str()) << "'" << endl;
    return false;
  }
  m_bootImage.resize((size_t)length);
  if (!m_bootImage.empty() && !file.read((char *)&m_bootImage[0], (streamsize)length)) {
    cerr << "starnet: could not read '" << path << "'" << endl;
    m_bootImage.clear();
    return false;
  }
  if (selected) {
    if (m_bootImage.size() > selected->m_length)
      m_bootImage.resize(selected->m_length);
  }
  else {
    // CP/M stores this image in 128-byte records and does not record the
    // exact byte length, so the file is padded with zeros. Keep through the
    // record that holds the last nonzero byte.
    size_t used = m_bootImage.size();
    while (used > 0 && m_bootImage[used - 1] == 0)
      --used;
    size_t kept = (used + Starnet::kRecordBytes - 1) / Starnet::kRecordBytes * Starnet::kRecordBytes;
    m_bootImage.resize(kept);
  }
  m_bootImagePath = path;
  cerr << "starnet: boot image " << path << endl;
  return true;
}

int StarnetServer::AlvBytes(int drive) const
{
  if (drive < 0 || drive >= kDpbSlots)
    return 0;

  // 56kbcpm.slv: per-drive ALV (next ALV / DIRBUF minus this ALV).
  static const int kAlv56[] = {
    0xdb68 - 0xdacf, // A: 153 bytes -> 1224 blocks
    0xdc01 - 0xdb68, // B: 153
    0xdc9a - 0xdc01, // C: 153
    0xdcb3 - 0xdc9a, // D: 25
    0xdce5 - 0xdcb3, // E: 50
    0,
  };
  // 64kbcpm.slv: A-D share ALV at DD6E (zero-filled through the BN
  // workspace at DF00). E-F reuse 25-byte stubs in the cold-boot area.
  static const int kAlv64[] = {
    0xdf00 - 0xdd6e, // A-D shared: 402 bytes -> 3216 blocks (~12.5MB @ 4K)
    0xdf00 - 0xdd6e,
    0xdf00 - 0xdd6e,
    0xdf00 - 0xdd6e,
    0xdc62 - 0xdc49, // E: 25 -> 200 blocks
    0xdc7b - 0xdc62, // F: 25
  };

  const BootImage * image = BootImageFor(m_bootImagePath);
  if (image != nullptr && strcmp(image->m_filename, "56kbcpm.slv") == 0)
    return kAlv56[drive];
  return kAlv64[drive];
}

bool StarnetServer::PrepareDrive(int drive)
{
  if (m_drives == nullptr || drive < 0 || drive >= CpmDriveSet::kDrives)
    return false;
  CpmDrive & d = (*m_drives)[drive];
  if (!d.Configured())
    return false;

  if (d.IsHost()) {
    int alv = AlvBytes(drive);
    if (alv > 0)
      d.SetMaxBlocks(alv * 8);
    if (!d.Refresh(nullptr))
      return false;
  }
  return d.Disk().m_spt > 0;
}

void StarnetServer::FillDpbs(uint8_t * dest)
{
  memset(dest, 0, (size_t)kDpbSlots * 15);
  if (m_drives == nullptr)
    return;

  for (int i = 0; i < kDpbSlots; ++i) {
    if (!PrepareDrive(i))
      continue;

    CpmDrive & drive = (*m_drives)[i];
    uint8_t dpb[15];
    drive.Dpb(dpb);
    // This client has no directory checksum vector. FS.COM stored zero here.
    dpb[11] = 0;
    dpb[12] = 0;
    memcpy(dest + (size_t)i * 15, dpb, 15);
    cerr << "starnet: DPB " << (char)('A' + i)
         << " SPT=" << drive.Disk().m_spt
         << " DSM=" << drive.Disk().m_dsm
         << " DRM=" << drive.Disk().m_drm
         << " OFF=" << drive.Disk().m_off
         << " ALVmax=" << AlvBytes(i) << endl;
  }
}

void StarnetServer::LoginProvidedDrives()
{
  memset(m_driveLoggedIn, 0, sizeof(m_driveLoggedIn));
  memset(m_driveReadOnly, 0, sizeof(m_driveReadOnly));
  if (m_drives == nullptr)
    return;

  for (int i = 0; i < CpmDriveSet::kDrives && i < kLoginDrives; ++i) {
    if (!PrepareDrive(i))
      continue;
    m_driveLoggedIn[i] = true;
    m_driveReadOnly[i] = true;
    cerr << "starnet: login " << (char)('A' + i) << ": RO" << endl;
  }
}

bool StarnetServer::ReadReservedRecord(const CpmDrive & drive, uint32_t record, uint8_t * dest) const
{
  if (drive.Disk().m_spt <= 0 || drive.Disk().m_off <= 0)
    return false;
  uint32_t reserved = (uint32_t)drive.Disk().m_off * (uint32_t)drive.Disk().m_spt;
  if (record >= reserved)
    return false;

  // Pad past the end of the slave with 0xE5, same as an empty host reserved track.
  memset(dest, 0xe5, Starnet::kRecordBytes);
  size_t offset = (size_t)record * Starnet::kRecordBytes;
  if (offset >= m_bootImage.size())
    return true;
  size_t n = min((size_t)Starnet::kRecordBytes, m_bootImage.size() - offset);
  memcpy(dest, &m_bootImage[offset], n);
  return true;
}

int StarnetServer::DriveNumber(const StarnetRequest & request) const
{
  return (int)(request.m_parm0 & 0xff);
}

bool StarnetServer::DriveLoggedIn(int number) const
{
  return number >= 0 && number < kLoginDrives && m_driveLoggedIn[number];
}

bool StarnetServer::DriveReadOnly(int number) const
{
  return number >= 0 && number < kLoginDrives && m_driveReadOnly[number];
}

CpmDrive * StarnetServer::DriveFor(const StarnetRequest & request)
{
  if (m_drives == nullptr)
    return nullptr;
  int number = DriveNumber(request);
  if (number < 0 || number >= CpmDriveSet::kDrives)
    return nullptr;
  CpmDrive & drive = (*m_drives)[number];
  if (!drive.Configured() || drive.Disk().m_spt <= 0)
    return nullptr;
  return &drive;
}

std::vector<StarnetFrame> StarnetServer::OnBoot(const StarnetRequest & request)
{
  const char * which = request.m_code == (uint8_t)Starnet::RequestType::WarmBoot ? "warm" : "cold";
  cerr << "starnet: " << which << " boot memsize " << hex << request.m_parm0 << dec << endl;
  if (!LoadBootImage())
    return { Reply(Starnet::ResponseType::RequestOutOfRange) };

  // The BN client uses one receive loop for both boots. Each packet names
  // the address to store and a nonzero parm1 while more packets follow.
  // The last packet carries the entry address and no payload. Cold boot
  // must enter the BIOS so it installs the jumps at 0000 and 0005. Warm
  // boot is requested by that BIOS and resumes at the CCP.
  vector<StarnetFrame> frames;
  size_t offset = 0;
  while (offset < m_bootImage.size()) {
    uint16_t len = (uint16_t)min(m_bootImage.size() - offset, (size_t)Starnet::kRecordBytes);
    frames.push_back(Reply(Starnet::ResponseType::Acknowledge,
                           (uint16_t)(kBootAddress + offset),
                           1,
                           0,
                           &m_bootImage[offset],
                           len));
    offset += len;
  }
  if (const BootImage * image = BootImageFor(m_bootImagePath)) {
    uint8_t dpb[kDpbSlots * 15];
    FillDpbs(dpb);
    uint16_t addr = (uint16_t)(kBootAddress + image->m_dpbOffset);
    frames.push_back(Reply(Starnet::ResponseType::Acknowledge,
                           addr,
                           1,
                           0,
                           dpb,
                           (uint16_t)sizeof(dpb)));
    cerr << "starnet: dpb packet at " << hex << addr << dec << endl;
  }
  // After FillDpbs so host volumes have a real SPT before login.
  LoginProvidedDrives();
  uint16_t entry = request.m_code == (uint8_t)Starnet::RequestType::WarmBoot
                       ? kBootAddress
                       : kBiosAddress;
  frames.push_back(Reply(Starnet::ResponseType::Acknowledge, entry, 0, 0));
  cerr << "starnet: " << which << " boot " << (frames.size() - 1) << " packets" << endl;
  return frames;
}

std::vector<StarnetFrame> StarnetServer::OnLogin(const StarnetRequest & request)
{
  int number = DriveNumber(request);
  if (number < 0 || number >= kLoginDrives || DriveFor(request) == nullptr)
    return { Reply(Starnet::ResponseType::BadDriveSpecification) };

  m_driveLoggedIn[number] = true;
  m_driveReadOnly[number] = true;
  cerr << "starnet: login " << (char)('A' + number) << ": RO"
       << " parm " << hex << request.m_parm1 << " " << request.m_parm2 << dec << endl;
  return { Reply(Starnet::ResponseType::Acknowledge) };
}

std::vector<StarnetFrame> StarnetServer::OnLogout(const StarnetRequest &)
{
  memset(m_driveLoggedIn, 0, sizeof(m_driveLoggedIn));
  memset(m_driveReadOnly, 0, sizeof(m_driveReadOnly));
  cerr << "starnet: logout" << endl;
  return { Reply(Starnet::ResponseType::Acknowledge) };
}

std::vector<StarnetFrame> StarnetServer::OnRead(const StarnetRequest & request)
{
  int number = DriveNumber(request);
  CpmDrive * drive = DriveFor(request);
  if (drive == nullptr)
    return { Reply(Starnet::ResponseType::BadDriveSpecification) };
  if (!DriveLoggedIn(number))
    return { Reply(Starnet::ResponseType::ReadWriteToADriveNotLoggedIn) };

  // parm1 is the track and the low byte of parm2 is the sector. A track
  // holds SPT records, taken from the DPB written into the boot image.
  uint32_t record = (uint32_t)request.m_parm1 * (uint32_t)drive->Disk().m_spt
                  + (uint32_t)(request.m_parm2 & 0xff);
  uint8_t data[Starnet::kRecordBytes];
  if (ReadReservedRecord(*drive, record, data)) {
    cerr << "starnet: reserved record " << record
         << " from boot image (" << (char)('A' + number) << ": "
         << request.m_parm1 << "/" << (request.m_parm2 & 0xff) << ")" << endl;
  }
  else if (!drive->ReadRecord(record, data)) {
    return { Reply(Starnet::ResponseType::ReadBeyondEndOfWorkSpace) };
  }
  return { Reply(Starnet::ResponseType::Acknowledge, 0, 0, 0, data, Starnet::kRecordBytes) };
}

std::vector<StarnetFrame> StarnetServer::OnWrite(const StarnetRequest & request)
{
  int number = DriveNumber(request);
  CpmDrive * drive = DriveFor(request);
  if (drive == nullptr)
    return { Reply(Starnet::ResponseType::BadDriveSpecification) };
  if (!DriveLoggedIn(number))
    return { Reply(Starnet::ResponseType::ReadWriteToADriveNotLoggedIn) };
  if (DriveReadOnly(number))
    return { Reply(Starnet::ResponseType::NoWritePermissionAndPermissionNotGrantable) };
  if (request.m_payload.size() != Starnet::kRecordBytes)
    return { Reply(Starnet::ResponseType::PhysicalReadWriteError) };

  uint32_t record = (uint32_t)request.m_parm1 * (uint32_t)drive->Disk().m_spt
                  + (uint32_t)(request.m_parm2 & 0xff);
  if (!drive->WriteRecord(record, request.m_payload.data()))
    return { Reply(Starnet::ResponseType::PhysicalReadWriteError) };
  return { Reply(Starnet::ResponseType::Acknowledge) };
}

std::vector<StarnetFrame> StarnetServer::OnUnimplemented(const StarnetRequest & request)
{
  cerr << "starnet: request '" << CommandName(request.m_code) << "' not yet implemented" << endl;
  return { Reply(Starnet::ResponseType::RequestOutOfRange) };
}
