#include "zed80.h"

#include <iostream>
#include <queue>
using namespace std;

#include <ncurses.h>

///////////////////////////////////////////////////////////////////
//
// TRS80 Model1
//

struct TRS80_Model1 : public Zed80
{
  TRS80_Model1()
  { 
  }

  bool Open()
  {
    if (!Zed80::Open(64))
      return false;

    if (!ReadROM("./roms/level2.rom", 0, 12*1024))
      return false;

    memset(&m_kbCode, 0, sizeof(m_kbCode));

    return true;
  }

  void VideoWrite(ushort offset, byte data)
  {
    int row = offset / 64;
    int col = offset % 64;

    if (data < 0x20)
      data += 0x40;

    mvprintw(row, col, "%c", (char)data);
    refresh();
  }

  void ReadChar(char ch)
  {
    m_kbQueue.push(ch);
  }

  byte HandleKeyPress(byte addr);

  std::queue<char> m_kbQueue;
  byte m_kbCode[8];
  byte m_kbState;
};

static byte CodeToBit[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

static void DoAlpha(char ch, byte * kbCode)
{
  if ((ch >= 'a') && (ch <= 'g'))
    kbCode[0] = CodeToBit[ch - 'a' + 1];
  else if ((ch >= 'h') && (ch <= 'o'))
    kbCode[1] = CodeToBit[ch - 'h'];
  else if ((ch >= 'p') && (ch <= 'w'))
    kbCode[2] = CodeToBit[ch - 'p'];
  else if ((ch >= 'x') && (ch <= 'z'))
    kbCode[3] = CodeToBit[ch - 'x'];
}

static void TranslateASCIIToCode(char ch, byte * kbCode)
{
  memset(kbCode, 0, 8);

  if (isalpha(ch)) {
    if (islower(ch))
      kbCode[7] = 0x01;
    DoAlpha(tolower(ch), kbCode);
  }
  else if ((ch >= '0') && (ch <= '7')) 
    kbCode[4] = CodeToBit[ch - '0'];
  else if ((ch >= '8') && (ch <= '9')) 
    kbCode[5] = CodeToBit[ch - '8'];
  else {
    switch (ch) {
      case 0x7f: // Delete -> Clear
        kbCode[6] = 0x02;
        break;
      case 0x0a: // Enter
        kbCode[6] = 0x01;
        break;
      case 0x20: // Space
        kbCode[6] = 0x80;
        break;
      case '@':
        kbCode[0] = 0x01;
      default:
        break;
    }
  }
}

byte TRS80_Model1::HandleKeyPress(byte addr)
{
  switch (m_kbState) { 
    case 0:
      if (m_kbQueue.size() == 0) 
        return 0x00;
      //printw("got char %i", m_kbQueue.front());
      TranslateASCIIToCode(m_kbQueue.front(), m_kbCode);
      m_kbQueue.pop();
      m_kbState++;
      break;
    case 10:
      memset(m_kbCode, 0, sizeof(m_kbCode));
      m_kbState++;
      break;
    case 20:
    default:
      m_kbState++;
      break;
  }

  return ((addr & 0x01) ? m_kbCode[0] : 0x00) |
         ((addr & 0x02) ? m_kbCode[1] : 0x00) |
         ((addr & 0x04) ? m_kbCode[2] : 0x00) |
         ((addr & 0x08) ? m_kbCode[3] : 0x00) |
         ((addr & 0x10) ? m_kbCode[4] : 0x00) |
         ((addr & 0x20) ? m_kbCode[5] : 0x00) |
         ((addr & 0x40) ? m_kbCode[6] : 0x00) |
         ((addr & 0x80) ? m_kbCode[7] : 0x00);
}

byte RdZ80(unsigned short address)
{
  TRS80_Model1 * system = static_cast<TRS80_Model1 *>(Zed80::g_instance);

  // ignore I/O space
  if (address < 0x3000 || address >= 0x3c00)
    return system->m_memory[address];

  // keyboard
  if ((address >= 0x3800) && (address <= 0x38ff))
    return system->HandleKeyPress(address & 0xff);

  //cout << "Read from " << hex << address << endl;

  return 0xff;
}

void WrZ80(register word address, register byte data)
{
  TRS80_Model1 * system = static_cast<TRS80_Model1 *>(Zed80::g_instance);

  if (address >= 0x3c00) {
    system->m_memory[address] = data;
    if (address >= 0x4000)
      return;
    system->VideoWrite(address - 0x3c00, data);
  }
}

byte InZ80(register word address)
{
  TRS80_Model1 * system = static_cast<TRS80_Model1 *>(Zed80::g_instance);
  cout << "Input from " << hex << address << endl;
  return 0x00;
}

void OutZ80(register word address, register byte data)
{
  TRS80_Model1 * system = static_cast<TRS80_Model1 *>(Zed80::g_instance);
}


int main(int argc, char ** argv)
{
  TRS80_Model1 trs80Model1;

  if (!trs80Model1.Open())
    return -1;

  initscr();
  noecho();
  cbreak();

  nodelay(stdscr, 1);
  curs_set(0);

  trs80Model1.Reset();

  while (trs80Model1.Run()) { 
    int ch = wgetch(stdscr);
    if (ch != ERR)
      trs80Model1.ReadChar(ch); 
  }

  curs_set(1);
  endwin();

  return 0;
}

