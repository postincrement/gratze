#include "model1.h"

Model1Level1_Emulator::Model1Level1_Emulator()
{
}

std::string Model1Level1_Emulator::GetTitle() const
{ 
  return "TRS-80 Model 1, Level 1"; 
}

int Model1Level1_Emulator::GetDefaultMemorySize_k() const
{ 
  return 4;
}

////////////////////////////////////////////////////////////////

extern unsigned char g_model1Level2ROM[12288];

Model1Level2_Emulator::Model1Level2_Emulator()
{
}

std::string Model1Level2_Emulator::GetTitle() const
{ 
  return "TRS-80 Model 1, Level 2"; 
}

int Model1Level2_Emulator::GetDefaultMemorySize_k() const
{ 
  return 48;
}
