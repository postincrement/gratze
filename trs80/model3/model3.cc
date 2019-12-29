#include "model3.h"

Model3_Emulator::Model3_Emulator()
{
}

std::string Model3_Emulator::GetTitle() const
{ 
  return "TRS-80 Model III"; 
}

int Model3_Emulator::GetDefaultMemorySize_k() const
{ 
  return 48;
}
