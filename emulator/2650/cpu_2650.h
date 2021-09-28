#ifndef CPU2650_H
#define CPU2650_H

struct CPU2650
{
  struct regset2650
  {
    unsigned char r[7]; // R0 R1 R2 R3 R1' R2' R3'
    unsigned char psl;        // program status lower
    unsigned char psu;        // program status upper
    unsigned short ap;        // address pointer
    unsigned short as[8];     // address stack
    unsigned short na;        // next address (not part of 2650 regsiter set)
    unsigned short ea;        // effective address (not part of 2650 regsiter set)
  };

  virtual ~CPU2650();
  
  regset2650 registers;

  unsigned char add(unsigned char byte1, unsigned char byte2);
  unsigned char sub(unsigned char byte1, unsigned char byte2);
  unsigned char com(unsigned char byte1, unsigned char byte2);
  unsigned char rrr(unsigned char byte);
  unsigned char rrl(unsigned char byte);
  void setcc(unsigned char byte);

  void push(unsigned short address);
  unsigned short pull(void);

  void interrupt(void);

  int cpu(void);
  void disassemble(void);

  virtual unsigned char ReadPortC() = 0;
  virtual void WritePortC(unsigned char data) = 0;

  virtual unsigned char ReadPortD() = 0;
  virtual void WritePortD(unsigned char data) = 0;

  virtual unsigned char ReadExtPort(unsigned char port) = 0;
  virtual void WritePortExt(unsigned char port, unsigned char data) = 0;

  virtual unsigned char ReadMemory(unsigned short) = 0;
  virtual void WriteMemory(unsigned short, unsigned char data) = 0;

  #define HISTORY_SIZE 10
  regset2650 history[HISTORY_SIZE];
  int historyindex;

  int cpucycles;
};


#endif // CPU2650_H