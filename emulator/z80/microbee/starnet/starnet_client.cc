#include "starnet_client.h"

#include <iomanip>
#include <iostream>

#include "udp_port.h"

using namespace std;

StarnetClient::StarnetClient()
  : m_port(new UdpPort)
{
  m_port->Connect(Starnet::kPort);
  Reset();
}

StarnetClient::~StarnetClient() = default;

void StarnetClient::SetStation(uint8_t station)
{
  m_station = station;
}

void StarnetClient::Reset()
{
  m_state = RxState::Idle;
  m_bodyLeft = 0;
  m_wire.clear();
  m_tx.clear();
  m_txPos = 0;
  m_frameSizes.clear();
}

void StarnetClient::Push(const uint8_t * bytes, size_t len)
{
  for (size_t i = 0; i < len; ++i)
    OnReceive(bytes[i]);
}

void StarnetClient::OnReceive(uint8_t byte)
{
  switch (m_state) {
    case RxState::Idle:
      // EDDC clocks a 0x00 before the length and the frame.
      if (byte == 0x00) {
        m_wire.clear();
        m_wire.push_back(byte);
        m_state = RxState::Length;
      }
      break;

    case RxState::Length:
      // The length byte is frame_len+1. The sender then clocks frame_len bytes.
      if (byte < (uint8_t)(Starnet::kHeaderBytes + 2) || byte > 0x8d) {
        m_state = RxState::Idle;
        m_wire.clear();
        break;
      }
      m_wire.push_back(byte);
      m_bodyLeft = (int)byte - 1;
      m_state = RxState::Body;
      break;

    case RxState::Body:
      m_wire.push_back(byte);
      if (--m_bodyLeft == 0) {
        m_state = RxState::Idle;
        Deliver();
      }
      break;
  }
}

void StarnetClient::Deliver()
{
  // m_wire is 0x00, length, then the request frame.
  uint8_t requestCode = 0;
  uint16_t parm0 = 0, parm1 = 0, parm2 = 0;
  if (m_wire.size() >= 3)
    requestCode = m_wire[2];
  if (m_wire.size() >= 9) {
    parm0 = (uint16_t)(m_wire[3] | (m_wire[4] << 8));
    parm1 = (uint16_t)(m_wire[5] | (m_wire[6] << 8));
    parm2 = (uint16_t)(m_wire[7] | (m_wire[8] << 8));
  }
  const bool boot = requestCode == (uint8_t)Starnet::RequestType::ColdBoot
                 || requestCode == (uint8_t)Starnet::RequestType::WarmBoot;
  if (!boot) {
    cerr << "starnet: client request " << (unsigned)requestCode
         << " parm " << hex << setw(4) << setfill('0') << parm0
         << ' ' << setw(4) << parm1
         << ' ' << setw(4) << parm2 << dec << endl;
  }

  std::vector<std::vector<uint8_t>> replies;
  uint8_t station = m_station;
  if (!m_port->Exchange(station, m_wire.data(), (int)m_wire.size(), replies)) {
    cerr << "starnet: client exchange failed code " << (unsigned)requestCode << endl;
    // The server did not know this workstation. Ask for a free slot.
    if (station == Starnet::kAssignStation && m_station != Starnet::kAssignStation) {
      m_station = Starnet::kAssignStation;
      station = m_station;
      if (!m_port->Exchange(station, m_wire.data(), (int)m_wire.size(), replies))
        return;
    }
    else
      return;
  }
  m_station = station;

  for (size_t i = 0; i < replies.size(); ++i) {
    const std::vector<uint8_t> & wire = replies[i];
    if (wire.size() < 2)
      continue;
    int frameLen = (int)wire[0] - 1;
    if (frameLen <= 0 || (int)wire.size() < 1 + frameLen)
      continue;
    QueueBytes(&wire[1], (uint16_t)frameLen);

    if (!boot) {
      if (frameLen >= 1)
        cerr << "starnet: client reply " << (unsigned)wire[1]
             << " len " << frameLen << endl;
      continue;
    }
    if (frameLen < Starnet::kHeaderBytes)
      continue;

    const uint8_t * frame = &wire[1];
    uint16_t addr = (uint16_t)(frame[1] | (frame[2] << 8));
    uint16_t more = (uint16_t)(frame[3] | (frame[4] << 8));
    int payload = frameLen - Starnet::kHeaderBytes - 1;
    const bool last = (i + 1 == replies.size());
    const bool dpb = addr == 0xd706 || addr == 0xd648;

    if (!last && !dpb)
      continue;

    cerr << "starnet: client "
         << (requestCode == (uint8_t)Starnet::RequestType::ColdBoot ? "cold" : "warm")
         << " boot packet " << (i + 1) << "/" << replies.size()
         << " addr " << hex << setw(4) << setfill('0') << addr
         << " more " << more
         << " payload " << dec << payload;
    if (dpb) {
      cerr << " dpb";
      int show = payload < 16 ? payload : 16;
      cerr << " [";
      for (int b = 0; b < show; ++b) {
        if (b)
          cerr << ' ';
        cerr << hex << setw(2) << setfill('0') << (unsigned)frame[Starnet::kHeaderBytes + b];
      }
      cerr << dec << "]";
    }
    if (last)
      cerr << " (last)";
    cerr << endl;
  }
}

void StarnetClient::QueueBytes(const uint8_t * bytes, uint16_t size)
{
  if (m_txPos > 0 && m_txPos == m_tx.size()) {
    m_tx.clear();
    m_txPos = 0;
  }
  m_frameSizes.push_back(size);
  m_tx.insert(m_tx.end(), bytes, bytes + size);
}

uint16_t StarnetClient::TakeFrameSize()
{
  if (m_frameSizes.empty())
    return 0;
  uint16_t size = m_frameSizes.front();
  m_frameSizes.pop_front();
  return size;
}

bool StarnetClient::HasTx() const
{
  return m_txPos < m_tx.size();
}

uint8_t StarnetClient::OnSend()
{
  if (!HasTx())
    return 0;
  return m_tx[m_txPos++];
}
