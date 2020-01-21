#include "assembler.h"

#include "s2650.h"



void Usage()
{
  cout << "usage: asm2650 fn" << endl;
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    Usage();
    return 0;
  }

  S2650Assembler assembler;

  if (!assembler.Open(argv[1])) {
    cerr << "error: " << assembler.GetError() << endl;
    return -1;
  }

  assembler.Parse();

  cout << assembler.GetLineCount() << " lines parsed" << endl;

  assembler.WriteListing();
  assembler.WriteBinary();
}

