#include <sstream>

#include "misc.h"

using namespace std;

std::string DumpMemory(const uint8_t * memory, int memoryLen)
{
  stringstream strm;

  int cols = 16;
  int p = 0;
  while (p < memoryLen) {
    strm << HEXFORMAT0x4(p) << "  ";
    int len = std::min(memoryLen-p, cols);
    int i;
    for (i = 0; i < len; ++i)
      strm << " " << HEXFORMAT2(memory[p + i]);
    while (i++ < cols)
      strm << "   ";
    strm << "   ";
    for (i = 0; i < len; ++i)
      strm << (isgraph(memory[p + i]) ? (char)memory[p + i] : '.');
    strm << endl;
    p += len;
  }

  return strm.str();
}
