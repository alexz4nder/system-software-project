#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <sys/types.h>

class Memory;
class Console;
class Timer;
class Proccessor;

class Bus {

  // void writeByte(int adr, char ch);
  // char readByte(int adr);

  Console *console;
  Timer *timer;
  Memory *memory;
  Proccessor *proccessor;

public:
  void write(uint32_t adr, uint32_t data);
  uint32_t read(uint32_t adr);

  enum class RegAdr {
    term_out = (int)0xffffff00,
    term_in = (int)0xffffff04,
    tim_cfg = (int)0xffffff10
  };

  Bus(char *filename);

  void interruptConsole();
  void interruptTimer();
};

#endif // !BUS_H
