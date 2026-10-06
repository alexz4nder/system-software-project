#define FMT_HEADER_ONLY
#include "../../incl/Bus.h"
#include "../../incl/Console.h"
#include "../../incl/Memory.h"
#include "../../incl/Proccessor.h"
#include "../../incl/Timer.h"

#include "../../incl/fmt/base.h"
#include <cstdint>
#include <cstdlib>
#include <sys/types.h>

Bus::Bus(char *filename) {
  console = new Console(this);
  timer = new Timer(this);
  memory = new Memory(filename);
  proccessor = new Proccessor(this);
  proccessor->gprs[15] = 0x40000000;
  while (true) {
    proccessor->doInstruction();
  }
}

void Bus::write(uint32_t adr, uint32_t data) {
  if (adr < 0xffffff00) {
    memory->write(adr, data);
    return;
  }
  // fmt::println("NON MEMORY:{}", adr);
  if (adr == (uint32_t)RegAdr::term_out) {
    console->writeConsoleOut(data);
    return;
  }
  if (adr == (uint32_t)RegAdr::tim_cfg) {
    timer->writeTimer(data);
    return;
  }
}

uint32_t Bus::read(uint32_t adr) {
  // fmt::println("BUS READ:{:#x}", adr);
  if (adr < 0xffffff00) {
    return memory->read(adr);
  }
  if (adr == (uint32_t)RegAdr::term_in) {
    return console->readConsoleIn();
  }

  return 0;
}

void Bus::interruptConsole() { proccessor->terminalInt = true; }
void Bus::interruptTimer() { proccessor->timerInt = true; }
