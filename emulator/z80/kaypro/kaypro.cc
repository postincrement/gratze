
#include <cstring>
#include <iostream>
#include <iomanip>
#include <functional>
#include <vector>

#include "common/misc.h"
#include "z80/kaypro/kaypro.h"
#include "video/virtual_screen.h"
#include "video/font.h"

using namespace std;

#define KAYPRO_CPU_MHZ          2.5
#define KAYPRO_ROM_START        0x0000
#define KAYPRO_ROM_END          0x0fff
#define KAYPRO_VRAM_START       0x3000
#define KAYPRO_VRAM_END         0x3fff
#define KAYPRO_COLS             80
#define KAYPRO_ROWS             25
#define KAYPRO_LINE_STRIDE      128
#define KAYPRO_FONT_W           8
#define KAYPRO_FONT_H           8
// Display cell height: 8x8 chargen is vertically stretched to fill this
// many scanlines (2× → 16) so glyphs look correctly proportioned.
#define KAYPRO_SCANLINES        16

static constexpr uint8_t kSystemBankMask   = 0x80;
static constexpr uint8_t kSystemDriveAMask = 0x01;
static constexpr uint8_t kSystemDriveBMask = 0x02;

/////////////////////////////////////////////////////////////////////////////////////////////

class KayproVideo : public MonoMemoryMappedScreen
{
  public:
    KayproVideo(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info);

    static bool CreatePixelFont(const Config::Font & fontInfo, std::vector<uint8_t> & fontData);

    FontChar GetCharAtPos(int x, int y) override;
    void WriteMemoryAtAddress(int addr, uint8_t ch) override;

    void SetScroll(uint8_t value);
    void SetScreenSize(int cols, int rows, int lines);

  private:
    uint8_t m_scroll = 0;
    int m_lines = KAYPRO_SCANLINES;
};

KayproVideo::KayproVideo(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info)
  : MonoMemoryMappedScreen(mainWindow, options, info)
{
}

bool KayproVideo::CreatePixelFont(const Config::Font & fontInfo, std::vector<uint8_t> & fontData)
{
  if (fontInfo.m_charGen == nullptr)
    return false;

  const int srcH = fontInfo.m_charGen->m_height;
  const int dstH = fontInfo.m_height;
  if ((srcH <= 0) || (dstH <= 0) || (dstH < srcH))
    return true;

  // Unpack left the 8 glyph rows at the top and blank-padded the rest.
  // Stretch those glyph rows across the full cell height so each source
  // scanline becomes multiple destination pixels.
  std::vector<uint8_t> glyph(srcH);
  for (int ch = 0; ch < fontInfo.m_count; ++ch) {
    uint8_t * cell = &fontData[ch * dstH];
    memcpy(glyph.data(), cell, srcH);
    for (int y = 0; y < dstH; ++y)
      cell[y] = glyph[(y * srcH) / dstH];
  }
  return true;
}

void KayproVideo::SetScroll(uint8_t value)
{
  m_scroll = value;
  RefreshScreen();
}

void KayproVideo::SetScreenSize(int cols, int rows, int lines)
{
  if (lines < 1)
    lines = 1;
  m_lines = lines;

  if (m_options.m_verbose)
    cout << "kaypro: setting screen to " << cols << "x" << rows
         << "x" << lines << endl;

  PixelFont * font = new PixelFont(g_charGen_kaypro80x25, 256, CreatePixelFont);
  font->SetHeight(lines);
  SetFont(font, cols, rows);
  RefreshScreen();
}

FontChar KayproVideo::GetCharAtPos(int x, int y)
{
  if ((x < 0) || (x >= KAYPRO_COLS) || (y < 0) || (y >= KAYPRO_ROWS))
    return 0x20;
  int addr = (y * KAYPRO_LINE_STRIDE + x + (m_scroll * KAYPRO_LINE_STRIDE)) & m_memoryMask;
  return m_memory[addr];
}

void KayproVideo::WriteMemoryAtAddress(int addr, uint8_t data)
{
  if ((addr < 0) || (addr >= (int)m_memory.size()))
    return;

  m_memory[addr] = data;

  int vis = (addr - (m_scroll * KAYPRO_LINE_STRIDE)) & m_memoryMask;
  int y = vis / KAYPRO_LINE_STRIDE;
  int x = vis % KAYPRO_LINE_STRIDE;
  if ((x < KAYPRO_COLS) && (y < KAYPRO_ROWS))
    RenderCharAtPos(x, y, m_cursorEnabled && (x == m_cursorX) && (y == m_cursorY), true);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Kaypro_Emulator::Instantiate()
{
  MemoryMappedScreen::AddType<KayproVideo>("kaypro");
}

Kaypro_Emulator::Kaypro_Emulator(const EmulatorInfo * info)
  : Z80Emulator(info)
{
}

bool Kaypro_Emulator::Open(const Options & options)
{
  if (!Z80Emulator::Open(options))
    return false;

  m_fdc.reset(new WD_FD1793(options.m_fdcDebug));

  auto * kb = new VirtualKeyboard();
  kb->SetKeyCharCallback([this](uint8_t ch) { OnKeyChar(ch); });
  SetKeyboard(kb);

  return true;
}

int Kaypro_Emulator::Exec(int cycles)
{
  // Hardware: NMI = (FDC DRQ | INTRQ) AND HALT. The ROM HALTs before each
  // FDC data byte / command completion; without that gate the NMI would fire
  // during OUT and be lost before HALT.
  int left = cycles;
  while (left > 0) {
    if (m_fdc && ((m_cpu.IFF & IFF_HALT) != 0) &&
        (m_fdc->GetDRQ() || m_fdc->GetInterrupt())) {
      NMI();
    }
    int rem = Z80Emulator::Exec(1);
    int ran = 1 - rem;
    if (ran < 1)
      ran = 1;
    left -= ran;
  }
  return left;
}

void Kaypro_Emulator::Reset(int addr)
{
  m_video = dynamic_cast<KayproVideo *>(m_screen.get());
  m_kbQueue.clear();
  m_sioControl = 0;
  m_sioControlWrites = 0;
  m_scroll = 0;
  if (m_video) {
    m_video->SetScroll(0);
    m_video->SetScreenSize(KAYPRO_COLS, KAYPRO_ROWS, KAYPRO_SCANLINES);
  }

  // Power-on: ROM + VRAM mapped (system bit 7 = 1).
  ApplySystemBits(0x80);

  if (m_fdc)
    m_fdc->Reset();

  Z80Emulator::Reset(addr);
}

bool Kaypro_Emulator::MountDrive(int driveNum, std::shared_ptr<VirtualDrive> drive, bool readOnly)
{
  if (!m_fdc)
    return false;
  return m_fdc->MountDrive(driveNum, drive, readOnly);
}

void Kaypro_Emulator::RefreshPanelDrives()
{
  CollectFdcDrives(m_fdc.get());
}

void Kaypro_Emulator::OnKeyChar(uint8_t ch)
{
  if (ch == 0x0a)
    ch = 0x0d;
  m_kbQueue.push_back(ch);
}

void Kaypro_Emulator::ApplySystemBits(uint8_t data)
{
  m_systemBits = data;
  m_romMapped = (data & kSystemBankMask) != 0;

  if (!m_fdc)
    return;

  // 81-232 prepare_drive selects with active-high drive bits (bit0=A, bit1=B).
  int drive = -1;
  if (data & kSystemDriveAMask)
    drive = 0;
  else if (data & kSystemDriveBMask)
    drive = 1;
  if (drive >= 0)
    m_fdc->SelectDrive(drive);

  // Side select (bit 2).
  m_fdc->SelectSide((data & 0x04) ? 1 : 0);
}

uint8_t Kaypro_Emulator::ReadIOMemory(int id, uint16_t addr) const
{
  if (id != KAYPRO_MEMORY_BANKED)
    return Z80Emulator::ReadIOMemory(id, addr);

  if (!m_romMapped)
    return m_lowRam[addr & 0x3fff];

  // Alternate map: ROM 0000-2FFF, VRAM 3000-3FFF.
  if (addr < KAYPRO_VRAM_START) {
    // 4K ROM mirrored through the 0000-2FFF window.
    return g_kaypro_81_232_ROM[addr & KAYPRO_ROM_END];
  }

  if (m_video)
    return m_video->ReadMemoryAtAddress(addr - KAYPRO_VRAM_START);
  return 0x00;
}

void Kaypro_Emulator::WriteIOMemory(int id, uint16_t addr, uint8_t val)
{
  if (id != KAYPRO_MEMORY_BANKED) {
    Z80Emulator::WriteIOMemory(id, addr, val);
    return;
  }

  if (!m_romMapped) {
    m_lowRam[addr & 0x3fff] = val;
    return;
  }

  // ROM window is read-only; writes fall through to underlying RAM so they
  // remain after the bank switches back to normal.
  if (addr < KAYPRO_VRAM_START) {
    m_lowRam[addr & 0x3fff] = val;
    return;
  }

  if (m_video)
    m_video->WriteMemoryAtAddress(addr - KAYPRO_VRAM_START, val);
}

uint8_t Kaypro_Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port)
{
  switch (info.m_id) {
    case KAYPRO_PORT_SIO: {
      // Kaypro: 0x05 = keyboard data, 0x07 = keyboard status (SIO B).
      // 0x04/0x06 are the modem channel — stub as idle.
      if (port == 0x05) {
        if (m_kbQueue.empty())
          return 0x00;
        uint8_t ch = m_kbQueue.front();
        m_kbQueue.pop_front();
        return ch;
      }
      if (port == 0x07)
        return m_kbQueue.empty() ? 0x00 : 0x01; // RR0 Rx char available
      if (port == 0x06)
        return 0x04; // Tx buffer empty
      return 0x00;
    }

    case KAYPRO_PORT_FDC:
      if (!m_fdc)
        return 0xff;
      return m_fdc->Read(port & 0x3);

    case KAYPRO_PORT_SCROLL:
      return m_scroll;

    case KAYPRO_PORT_SYSTEM:
      // Centronics ready (bit 3) idle-high; return latched bits otherwise.
      return (uint8_t)(m_systemBits | 0x08);

    case KAYPRO_PORT_PARALLEL:
      return 0xff;

    case KAYPRO_PORT_BAUD:
      return 0xff;
  }

  cerr << "kaypro: read port " << HEXFORMAT0x2(port) << endl;
  return 0xff;
}

void Kaypro_Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  switch (info.m_id) {
    case KAYPRO_PORT_SIO:
      // Ignore SIO programming; keyboard is polled via 0x05/0x07.
      if ((port == 0x06) || (port == 0x07)) {
        m_sioControl = data;
        ++m_sioControlWrites;
      }
      return;

    case KAYPRO_PORT_FDC:
      if (m_fdc)
        m_fdc->Write(port & 0x3, data);
      return;

    case KAYPRO_PORT_SCROLL:
      m_scroll = data;
      if (m_video)
        m_video->SetScroll(data);
      return;

    case KAYPRO_PORT_SYSTEM:
      // 0x1C = data, 0x1D = control (PIO). Only data updates bank/drives.
      if ((port & 0x03) == 0)
        ApplySystemBits(data);
      return;

    case KAYPRO_PORT_PARALLEL:
    case KAYPRO_PORT_BAUD:
      return;
  }

  cerr << "kaypro: write port " << HEXFORMAT0x2(port) << " " << HEXFORMAT0x2(data) << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////

INFO_START(kaypro)
{
  INFO_MONITOR(12.0, 4.0, 3.0, eNTSC),

  INFO_IO_PORT_RW(0x00, 0x00, KAYPRO_PORT_BAUD),
  INFO_IO_PORT_RW(0x04, 0x07, KAYPRO_PORT_SIO),
  INFO_IO_PORT_RW(0x08, 0x0b, KAYPRO_PORT_PARALLEL),
  INFO_IO_PORT_RW(0x0c, 0x0f, KAYPRO_PORT_BAUD),
  INFO_IO_PORT_RW(0x10, 0x13, KAYPRO_PORT_FDC),
  INFO_IO_PORT_RW(0x14, 0x14, KAYPRO_PORT_SCROLL),
  INFO_IO_PORT_RW(0x1c, 0x1f, KAYPRO_PORT_SYSTEM),
}
INFO_END(kaypro);

/////////////////////////////////////////////////////////////////////////////////////////////

extern EmulatorInfo g_kaypro2EmulatorInfo;

Kaypro2_Emulator::Kaypro2_Emulator()
  : Kaypro_Emulator(&g_kaypro2EmulatorInfo)
{}

INFO_START(kaypro2)
{
  INFO_CPU(KAYPRO_CPU_MHZ, KAYPRO_ROM_START),
  INFO_PARENT(kaypro),

  INFO_MEM_IO(0x0000, 0x3fff, KAYPRO_MEMORY_BANKED),
  INFO_MAIN_RAM(0x4000, 48, 48, 48),

  INFO_SCREEN_MEMORY_MAPPED_VARIABLE("kaypro", 0x1000,
                        KAYPRO_COLS, KAYPRO_ROWS,
                        KAYPRO_FONT_W, KAYPRO_FONT_H,
                        256,
                        &g_charGen_kaypro80x25, nullptr),
}
INFO_END(kaypro2);

EmulatorInfo g_kaypro2EmulatorInfo =
{
  "kaypro2",
  "Kaypro 2",
  "Kaypro II",
  true,

  INFO_INSERT(kaypro2)
};

/////////////////////////////////////////////////////////////////////////////////////////////

KayproIV_Emulator::KayproIV_Emulator()
  : Kaypro_Emulator(&g_kaypro2EmulatorInfo)
{}