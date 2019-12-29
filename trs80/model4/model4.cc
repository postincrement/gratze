#include "model4.h"

Model4_Emulator::Model4_Emulator()
{
}

std::string Model4_Emulator::GetTitle() const
{ 
  return "TRS-80 Model IV"; 
}

int Model4_Emulator::GetDefaultMemorySize_k() const
{ 
  return 48;
}
