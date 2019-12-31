#include "model4.h"

extern uint8_t g_trs80CharSets_256[][256][MODEL3_FONT_HEIGHT];

Model4_Emulator::Model4_Emulator()
{
}

std::string Model4_Emulator::GetTitle() const
{ 
  return "TRS-80 Model IV"; 
}

int Model4_Emulator::GetDefaultRAMSize_k() const
{ 
  return 48;
}

int Model4_Emulator::GetVideoMemSize_k()
{
  return 2048;
}

int Model4_Emulator::GetScreenWidth() const
{
  return MODEL4_SCREEN_WIDTH;
}

int Model4_Emulator::GetScreenHeight() const
{
  return MODEL4_SCREEN_HEIGHT;
}

double Model4_Emulator::GetTargetClockSpeed_Hz() const
{
  return 2000000;
}

bool Model4_Emulator::OpenVideo(MainWindow & mainWindow, const Options & options)
{
  if (!Z80Emulator::OpenVideo(mainWindow,  options) || !m_video)
    return false;

  // set the font
  CreateFontData(MODEL4_FONT_WIDTH, MODEL4_FONT_HEIGHT, g_trs80CharSets_256[7-4][0]);

  return true;
}





