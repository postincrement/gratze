#include "starnet_session.h"

#include <vector>

namespace {

bool Take(StarnetPort & port, std::vector<uint8_t> & got, int need)
{
  uint8_t buf[512];
  while ((int)got.size() < need) {
    int n = port.Read(buf, (int)sizeof(buf));
    if (n <= 0)
      return false;
    got.insert(got.end(), buf, buf + n);
  }
  return true;
}

} // namespace

std::vector<uint8_t> StarnetSession::WireReply(const StarnetFrame & frame)
{
  std::vector<uint8_t> body;
  body.push_back(frame.m_code);
  body.push_back((uint8_t)(frame.m_parm0 & 0xff));
  body.push_back((uint8_t)((frame.m_parm0 >> 8) & 0xff));
  body.push_back((uint8_t)(frame.m_parm1 & 0xff));
  body.push_back((uint8_t)((frame.m_parm1 >> 8) & 0xff));
  body.push_back((uint8_t)(frame.m_parm2 & 0xff));
  body.push_back((uint8_t)((frame.m_parm2 >> 8) & 0xff));
  body.insert(body.end(), frame.m_payload.begin(), frame.m_payload.end());
  body.push_back(StarnetRxChecksum(body.data(), body.size()));

  // EE50 reads this length into B, decrements it, and INIRs the frame.
  std::vector<uint8_t> wire;
  wire.push_back((uint8_t)(body.size() + 1));
  wire.insert(wire.end(), body.begin(), body.end());
  return wire;
}

bool StarnetSession::Serve(StarnetServer & server, StarnetPort & port)
{
  // EDDC clocks 0x00, then frame_len+1, then the frame.
  std::vector<uint8_t> got;
  if (!Take(port, got, 2))
    return false;
  if (got[0] != 0x00)
    return false;

  int frameLen = (int)got[1] - 1;
  if (frameLen < Starnet::kHeaderBytes + 1 || frameLen > 0x8c)
    return false;
  if (!Take(port, got, 2 + frameLen))
    return false;

  const uint8_t * frame = got.data() + 2;
  std::vector<StarnetFrame> replies;
  if (frame[frameLen - 1] != StarnetRxChecksum(frame, (size_t)frameLen - 1)) {
    StarnetFrame error;
    error.m_code = (uint8_t)Starnet::ResponseType::ChecksumErrorOnReceivedBlock;
    replies.push_back(error);
  }
  else {
    StarnetRequest request;
    request.m_code = frame[0];
    request.m_parm0 = (uint16_t)(frame[1] | (frame[2] << 8));
    request.m_parm1 = (uint16_t)(frame[3] | (frame[4] << 8));
    request.m_parm2 = (uint16_t)(frame[5] | (frame[6] << 8));
    if (frameLen > Starnet::kHeaderBytes + 1)
      request.m_payload.assign(frame + Starnet::kHeaderBytes, frame + frameLen - 1);
    replies = server.Handle(request);
  }

  for (const StarnetFrame & reply : replies) {
    std::vector<uint8_t> wire = WireReply(reply);
    if (!port.WaitForReceiver())
      return false;
    if (port.Write(wire.data(), (int)wire.size()) != (int)wire.size())
      return false;
  }
  return true;
}
