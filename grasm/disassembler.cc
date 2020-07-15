#include <sstream>
#include <iomanip>
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "disassembler.h"
#include "common/cmdargs.h"
#include "common/misc.h"

using namespace std;

Disassembler::Disassembler(const FormatInfo & format)
  : XSsembler(format)
{}


std::string GetMnemonic(const Disassembler::OpCodeInfo * info, uint8_t opcode)
{
  std::string mnem = info->m_mnemonic;
  if (info->m_bitInfo == 0)
    return mnem;

  int mask = info->m_mask;
  std::string extra;
  if (mask == 0xff) {
    extra = (const char *)info->m_bitInfo;
  }
  else if (info->m_mode & XSsembler::eFunc) {
    typedef std::string (* Func)(uint8_t);
    Func func = (Func)info->m_bitInfo;
    mnem += " " + (*func)(opcode);
  } 
  else {
    const char ** strings = (const char **)info->m_bitInfo;

    // count number of strings
    int stringCount = 0;
    while (strings[stringCount] != 0)
      ++stringCount;

    // shift opcode down so mask is zero
    int bits = 0;
    while ((mask & 0x01) != 0) {
      mask = (mask >> 1) | 0x80;
      opcode = opcode >> 1;
      bits++;
    }

    // get string, making sure not to go past the end
    extra = ((const char **)info->m_bitInfo)[(opcode & ~mask) % stringCount];
    if (!extra.empty())
      mnem += " " + extra;
  }

  return mnem;
}


void Disassembler::Matrix()
{
  const OpCodeInfo * opcodes = GetOpcodes();
  ColumnFormatter::Columns<17> cols;
  cols[0].push_back(" ");
  for (int col = 0; col < 16; col++) {
    stringstream strm;
    strm << hex << (int)(col*16);
    cols[col+1].push_back(strm.str());
  }
  for (int row = 0; row < 16; row++) {
    {
      stringstream strm;
      strm << hex << (int)(row);
      cols[0].push_back(strm.str());
    }
    for (int col = 0; col < 16; col ++) {
      uint8_t opcode = (col * 16) + row;
      const OpCodeInfo * info = nullptr;
      const OpCodeInfo * ptr = GetOpcodes();

      // find best match
      while (ptr->m_mnemonic != 0) {
        if (
          ((opcode & ptr->m_mask) == ptr->m_opcode) &&
          ((info == nullptr) || (ptr->m_mask > info->m_mask))
          )
          info = ptr;
        ptr++;     
      }

      std::string mnem;
      if (info != nullptr)
        mnem = GetMnemonic(info, opcode);
      cols[col+1].push_back(mnem);
    }
  }

  cout << ColumnFormatter::Print<17>(" | ", cols);
}

bool Disassembler::Open(const CommandLineArgs & args, const std::string & fn)
{
  int fd = ::open(fn.c_str(), O_RDONLY);
  if (fd < 0) {
    cerr << "error: cannot open '" << fn << "' - " << strerror(errno) << endl;
    return false;
  }

  m_origin = 0;
  args.GetValue("--org", m_origin);

  struct stat stbuf;
  if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
    cerr << "error: '" << fn << "' inaccessible or not a regular file" << endl;
    return false;
  }  
  off_t len = stbuf.st_size;  

  // allocate and read data
  m_image.resize(len);
  lseek(fd, 0, SEEK_SET);
  ::read(fd, &m_image[0], len);
  ::close(fd);

  cerr << "info: read " << len << " bytes" << endl;

  std::vector<unsigned> dbs;
  if (args.GetValues("--db", dbs)) {
    for (auto & r : dbs)
      m_forceDb.insert(r);
  }

  return true;
}


bool Disassembler::Run()
{
  std::stringstream strm;

  // two passes through the source
  for (m_pass = 1; m_pass <= 2; ++m_pass) {
    unsigned offset = 0;
    bool wasDb = false;
    bool wasTerm = false;
    strm.str("");
    while (offset < m_image.size()) {
      int insLen = 0;
      std::string text;
      bool forced = m_forceDb.count(m_origin + offset);

      // decode the opcode
      DisasmInfo disasm;   // symbol, mnem, args, comment
      unsigned address = m_origin + offset;
      bool isDb = false;
      bool isTerm = false;
      if (!forced && DecodeInstruction(disasm, insLen, address, m_image, offset, isTerm) && (insLen != 0)) {
        isDb = false;
      }
      else {
        isDb = true;
        insLen = 1;
        disasm[0] = "";
        disasm[1] = "db";
        disasm[2] = strm.str();
        disasm[3] = forced ? "forced" : "";
      }

      // output something the assembler should accept
      auto r = m_reverseSymbols.find(address);
      if (r != m_reverseSymbols.end())
        disasm[0] = r->second;

      if (wasTerm || (isDb != wasDb)) {
        strm << endl;
        wasDb = isDb;
      }
      wasTerm = isTerm;

      strm << disasm[0] << std::string(std::max((int)(m_format.m_symColWidth  - disasm[0].length()), 1), ' '); 
      strm << disasm[1] << std::string(std::max((int)(m_format.m_mnemColWidth - disasm[1].length()), 1), ' ');
      strm << disasm[2] << std::string(std::max((int)(m_format.m_argColWidth  - disasm[2].length()), 1), ' ');
      strm << "; "<< setw(4) << setfill('0') << hex << address << " ";

      int j;
      for (j = 0; j < insLen; ++j)
        strm << setw(2) << setfill('0') << hex << (int)m_image[offset+j] << " ";
      for (;j < m_format.m_opcodeByteCount; ++j)
        strm << "   ";    
      strm << "  ";

      for (j = 0; j < insLen; ++j)
        strm << (isgraph(m_image[offset+j]) ? (char)m_image[offset+j] : '.');
      for (;j < m_format.m_opcodeByteCount; ++j)
        strm << " ";        
      strm << "  ";

      strm << disasm[4] << endl;

      offset += insLen;
    }
  }

  // output symbols not in range of the binary
  for (auto & r : m_reverseSymbols) {
    unsigned addr      = r.first;
    std::string symbol = r.second;
//    if ((addr < m_origin) || (addr >= (m_origin + m_image.size()))) {
      strm << symbol << std::string(std::max((int)(m_format.m_symColWidth - symbol.length()), 1), ' '); 
      strm << "equ"  << std::string(std::max((int)(m_format.m_mnemColWidth - 3), 1), ' ');
      strm << "0"    << setw(4) << setfill('0') << addr << "h" << endl;
//    }  
  }

  cout << strm.str();

  return false;
}

std::string Disassembler::CreateSymbol(unsigned addr)
{
  stringstream strm;
  strm << "L" << setw(4) << setfill('0') << hex << addr;
  AssignSymbol(strm.str(), addr);
  m_reverseSymbols.insert(ReverseSymbolTable::value_type(addr, strm.str()));
  return strm.str();
}

