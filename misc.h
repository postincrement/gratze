#ifndef MISC_H_
#define MISC_H_

#include <iostream>
#include <iomanip>


#define   HEXFORMAT2(val) std::hex << std::setw(2) << std::setfill('0') << ((unsigned int)(val) & 0xff) << std::dec
#define   HEXFORMAT4(val) std::hex << std::setw(4) << std::setfill('0') << ((unsigned int)(val) & 0xffff) << std::dec
#define   HEXFORMAT8(val) std::hex << std::setw(8) << std::setfill('0') << ((unsigned int)(val)) << std::dec

#define   HEXFORMAT0x2(val) "0x" << HEXFORMAT2(val)
#define   HEXFORMAT0x4(val) "0x" << HEXFORMAT4(val)
#define   HEXFORMAT0x8(val) "0x" << HEXFORMAT8(val)

#endif // MISC_H_