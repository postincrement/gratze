#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

#include <unistd.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include "starnet_server.h"

using namespace std;

namespace {

const char * kBootImage = "z80/microbee/starnet/64kbcpm.slv";
// 64kbcpm.slv is a CP/M system: CCP at C000, BDOS at C800, BIOS at D600.
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
string FindBootImage()
{
  vector<string> candidates;
  candidates.push_back(string("./") + kBootImage);
  candidates.push_back(string("./emulator/") + kBootImage);

  string dir = ExecutableDir();
  for (int i = 0; i < 6 && !dir.empty(); ++i) {
    candidates.push_back(dir + "/" + kBootImage);
    candidates.push_back(dir + "/emulator/" + kBootImage);
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

StarnetServer::StarnetServer(string bootImagePath, string workspacePath)
  : m_bootImagePath(std::move(bootImagePath))
  , m_workspacePath(std::move(workspacePath))
{
  if (m_workspacePath.empty())
    m_workspacePath = "starnet.workspace";

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
  m_loggedIn = false;
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
  if (it == m_commands.end()) {
    cerr << "starnet: request " << (int)request.m_code << " out of range" << endl;
    return { Reply(Starnet::ResponseType::RequestOutOfRange) };
  }
  return it->second.m_handler(request);
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
  if (!m_bootImage.empty())
    return true;

  string path = m_bootImagePath.empty() ? FindBootImage() : m_bootImagePath;
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
    cerr << "starnet: could not read '" << (path.empty() ? kBootImage : path) << "'" << endl;
    return false;
  }
  m_bootImage.resize((size_t)length);
  if (!m_bootImage.empty() && !file.read((char *)&m_bootImage[0], (streamsize)length)) {
    cerr << "starnet: could not read '" << path << "'" << endl;
    m_bootImage.clear();
    return false;
  }
  m_bootImagePath = path;
  return true;
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
  uint16_t entry = request.m_code == (uint8_t)Starnet::RequestType::WarmBoot
                       ? kBootAddress
                       : kBiosAddress;
  frames.push_back(Reply(Starnet::ResponseType::Acknowledge, entry, 0, 0));
  cerr << "starnet: " << which << " boot " << (frames.size() - 1) << " packets" << endl;
  return frames;
}

std::vector<StarnetFrame> StarnetServer::OnLogin(const StarnetRequest & request)
{
  m_loggedIn = true;
  cerr << "starnet: login drive " << (request.m_parm0 & 0xff)
       << " parm " << hex << request.m_parm1 << " " << request.m_parm2 << dec << endl;
  return { Reply(Starnet::ResponseType::Acknowledge) };
}

std::vector<StarnetFrame> StarnetServer::OnLogout(const StarnetRequest &)
{
  m_loggedIn = false;
  cerr << "starnet: logout" << endl;
  return { Reply(Starnet::ResponseType::Acknowledge) };
}

uint32_t StarnetServer::RecordNumber(const StarnetRequest & request) const
{
  // BIOS SETTRK stores BC in parm1 and SETSEC stores the low byte in parm2.
  // 128 sectors per track matches the 128-byte records this BIOS moves.
  return (uint32_t)request.m_parm1 * 128u + (uint32_t)(request.m_parm2 & 0xff);
}

bool StarnetServer::OpenWorkspace()
{
  ifstream existing(m_workspacePath, ios::binary);
  if (existing.good())
    return true;
  ofstream created(m_workspacePath, ios::binary);
  return (bool)created;
}

std::vector<StarnetFrame> StarnetServer::OnRead(const StarnetRequest & request)
{
  if (!m_loggedIn)
    return { Reply(Starnet::ResponseType::NotLoggedIn) };

  uint32_t record = RecordNumber(request);
  uint64_t offset = (uint64_t)record * Starnet::kRecordBytes;
  ifstream file(m_workspacePath, ios::binary);
  if (!file)
    return { Reply(Starnet::ResponseType::ReadBeyondEndOfWorkSpace) };
  file.seekg(0, ios::end);
  auto size = file.tellg();
  if (size < 0 || (uint64_t)size < offset + Starnet::kRecordBytes)
    return { Reply(Starnet::ResponseType::ReadBeyondEndOfWorkSpace) };

  file.seekg((std::streamoff)offset);
  uint8_t data[Starnet::kRecordBytes];
  if (!file.read((char *)data, Starnet::kRecordBytes))
    return { Reply(Starnet::ResponseType::PhysicalReadWriteError) };
  return { Reply(Starnet::ResponseType::Acknowledge, 0, 0, 0, data, Starnet::kRecordBytes) };
}

std::vector<StarnetFrame> StarnetServer::OnWrite(const StarnetRequest & request)
{
  if (!m_loggedIn)
    return { Reply(Starnet::ResponseType::NotLoggedIn) };
  if (request.m_payload.size() != Starnet::kRecordBytes)
    return { Reply(Starnet::ResponseType::PhysicalReadWriteError) };
  if (!OpenWorkspace())
    return { Reply(Starnet::ResponseType::PhysicalReadWriteError) };

  uint32_t record = RecordNumber(request);
  uint64_t offset = (uint64_t)record * Starnet::kRecordBytes;
  fstream file(m_workspacePath, ios::in | ios::out | ios::binary);
  if (!file)
    return { Reply(Starnet::ResponseType::PhysicalReadWriteError) };
  file.seekp((std::streamoff)offset);
  if (!file.write((const char *)&request.m_payload[0], Starnet::kRecordBytes))
    return { Reply(Starnet::ResponseType::PhysicalReadWriteError) };
  file.flush();
  if (!file)
    return { Reply(Starnet::ResponseType::PhysicalReadWriteError) };
  return { Reply(Starnet::ResponseType::Acknowledge) };
}

std::vector<StarnetFrame> StarnetServer::OnUnimplemented(const StarnetRequest & request)
{
  cerr << "starnet: request '" << CommandName(request.m_code) << "' not yet implemented" << endl;
  return { Reply(Starnet::ResponseType::RequestOutOfRange) };
}
