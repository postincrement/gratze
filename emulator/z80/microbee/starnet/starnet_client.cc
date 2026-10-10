#include "starnet_client.h"

StarnetClient::StarnetClient(StarnetServer & server)
  : m_server(server)
{
  Reset();
}

void StarnetClient::Reset()
{
  m_state = RxState::Idle;
  m_got = 0;
  m_payloadNeed = 0;
  m_payload.clear();
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
      // EDDC clocks a 0x00 and a length before the frame, under mode 0.
      // A frame pushed for debugging starts with the request code.
      if (byte == 0x00) {
        m_state = RxState::SkipLength;
        return;
      }
      m_got = 0;
      m_payload.clear();
      m_state = RxState::Header;
      AcceptHeaderByte(byte);
      break;

    case RxState::SkipLength:
      m_state = RxState::Idle;
      break;

    case RxState::Header:
      AcceptHeaderByte(byte);
      break;

    case RxState::Payload:
      m_payload.push_back(byte);
      if (m_payload.size() >= m_payloadNeed)
        m_state = RxState::Checksum;
      break;

    case RxState::Checksum:
      FinishFrame(byte);
      m_state = RxState::Idle;
      break;
  }
}

void StarnetClient::AcceptHeaderByte(uint8_t byte)
{
  m_header[m_got++] = byte;
  if (m_got < Starnet::kHeaderBytes)
    return;

  uint16_t parm0 = (uint16_t)(m_header[1] | (m_header[2] << 8));
  uint16_t parm1 = (uint16_t)(m_header[3] | (m_header[4] << 8));
  uint16_t parm2 = (uint16_t)(m_header[5] | (m_header[6] << 8));
  m_payloadNeed = m_server.PayloadLength(m_header[0], parm0, parm1, parm2);
  m_payload.clear();
  if (m_payloadNeed == 0)
    m_state = RxState::Checksum;
  else {
    m_payload.reserve(m_payloadNeed);
    m_state = RxState::Payload;
  }
}

void StarnetClient::FinishFrame(uint8_t checksum)
{
  std::vector<uint8_t> body(m_header, m_header + Starnet::kHeaderBytes);
  body.insert(body.end(), m_payload.begin(), m_payload.end());
  if (checksum != StarnetRxChecksum(body.data(), body.size())) {
    StarnetFrame error;
    error.m_code = (uint8_t)Starnet::ResponseType::ChecksumErrorOnReceivedBlock;
    Queue(error);
    return;
  }

  StarnetRequest request;
  request.m_code = m_header[0];
  request.m_parm0 = (uint16_t)(m_header[1] | (m_header[2] << 8));
  request.m_parm1 = (uint16_t)(m_header[3] | (m_header[4] << 8));
  request.m_parm2 = (uint16_t)(m_header[5] | (m_header[6] << 8));
  request.m_payload = std::move(m_payload);
  Queue(m_server.Handle(request));
}

void StarnetClient::Queue(const std::vector<StarnetFrame> & frames)
{
  for (const StarnetFrame & frame : frames)
    Queue(frame);
}

void StarnetClient::Queue(const StarnetFrame & frame)
{
  if (m_txPos > 0 && m_txPos == m_tx.size()) {
    m_tx.clear();
    m_txPos = 0;
  }

  std::vector<uint8_t> bytes;
  bytes.push_back(frame.m_code);
  bytes.push_back((uint8_t)(frame.m_parm0 & 0xff));
  bytes.push_back((uint8_t)((frame.m_parm0 >> 8) & 0xff));
  bytes.push_back((uint8_t)(frame.m_parm1 & 0xff));
  bytes.push_back((uint8_t)((frame.m_parm1 >> 8) & 0xff));
  bytes.push_back((uint8_t)(frame.m_parm2 & 0xff));
  bytes.push_back((uint8_t)((frame.m_parm2 >> 8) & 0xff));
  bytes.insert(bytes.end(), frame.m_payload.begin(), frame.m_payload.end());
  bytes.push_back(StarnetRxChecksum(bytes.data(), bytes.size()));
  m_frameSizes.push_back((uint16_t)bytes.size());
  m_tx.insert(m_tx.end(), bytes.begin(), bytes.end());
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
