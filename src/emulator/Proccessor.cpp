#define FMT_HEADER_ONLY
#include "../../incl/Proccessor.h"
#include "../../incl/Bus.h"
#include "../../incl/InstructionProccessing.h"
#include "../../incl/fmt/base.h"
#include "../../incl/fmt/core.h"
#include <cstdint>
#include <cstdlib>
#include <iostream>

Instruction::Instruction() {}
Proccessor::Proccessor(Bus *bus) {
  this->bus = bus;
  this->gprs[0] = 0;
  // this->specrs[statusIndex] = 0b111;
  this->specrs[statusIndex] = 0;
}
void Proccessor::doInstruction() {
  uint32_t pc = gprs[pcIndex];
  gprs[pcIndex] += 4;

  Instruction inst;
  *((uint32_t *)&inst) = bus->read(pc);

  // uncoment to run in steps
  // char ch;
  // std::cin >> ch;

  // fmt::println("SP:{:#x} topOfStack:{:#x}", gprs[14], bus->read(gprs[14]));
  // fmt::println("R1:{:#x}", gprs[1]);
  // fmt::println("R2:{:#x}", gprs[2]);
  //  fmt::println("R3:{:#x}", gprs[3]);
  // fmt::println("PC:{:#x} INTRUCTION:{}", pc, inst); //     fmt::println("RAW
  //    INST:{:#x}", *((uint32_t *)&inst));

  // instruction execution
  switch (inst.opcode) {
  case 0b0000:
    halt(inst);
    break;
  case 0b0001:
    softwareInt(inst);
    break;
  case 0b0010:
    subRoutine(inst);
    break;
  case 0b0011:
    jumpOps(inst);
    break;
  case 0b0100:
    atomicSwap(inst);
    break;
  case 0b0101:
    arithmeticOps(inst);
    break;
  case 0b110:
    logicOps(inst);
    break;
  case 0b111:
    bitShift(inst);
    break;
  case 0b1000:
    st(inst);
    break;
  case 0b1001:
    ld(inst);
    break;
  default:
    ilInt = true;
    break;
  }

  gprs[0] = 0;
  // fmt::println("PC AFTER INC:{:#x}", gprs[pcIndex]);

  checkAndHandleInterrupt();
}

void Proccessor::checkAndHandleInterrupt() {
  if (syscallInt) {
    fmt::println("SYSCALL INTERRUPT");
    //  push status and pc to stack
    gprs[spIndex] -= 4;
    bus->write(gprs[spIndex], specrs[statusIndex]);
    gprs[spIndex] -= 4;
    bus->write(gprs[spIndex], gprs[pcIndex]);

    // disable interrupts
    specrs[statusIndex] |= 0b100;

    specrs[causeIndex] = 4;
    gprs[pcIndex] = specrs[handlerIndex];

    syscallInt = false;
    return;
  }

  if (ilInt) { // illegeal instruction
    // fmt::println("ILLEGAL INSTRUCTION INTERRUPT");
    //   push status and pc to stack
    gprs[spIndex] -= 4;
    bus->write(gprs[spIndex], specrs[statusIndex]);
    gprs[spIndex] -= 4;
    bus->write(gprs[spIndex], gprs[pcIndex]);

    // disable interrupts
    specrs[statusIndex] |= 0b100;

    specrs[causeIndex] = 1;
    gprs[pcIndex] = specrs[handlerIndex];

    ilInt = false;
    return;
  }

  if ((specrs[statusIndex] & 0b100) == 0) {
    if (timerInt && (specrs[statusIndex] & 0b1) == 0) {
      // push status and pc to stack
      gprs[spIndex] -= 4;
      bus->write(gprs[spIndex], specrs[statusIndex]);
      gprs[spIndex] -= 4;
      bus->write(gprs[spIndex], gprs[pcIndex]);

      // disable interrupts
      specrs[statusIndex] |= 0b100;

      specrs[causeIndex] = 2;
      gprs[pcIndex] = specrs[handlerIndex];

      timerInt = false;
      // fmt::println("TIMER INT");
      return;
    }
    if (terminalInt && (specrs[statusIndex] & 0b10) == 0) {
      // push status and pc to stack
      gprs[spIndex] -= 4;
      bus->write(gprs[spIndex], specrs[statusIndex]);
      gprs[spIndex] -= 4;
      bus->write(gprs[spIndex], gprs[pcIndex]);

      // disable interrupts
      specrs[statusIndex] |= 0b100;

      specrs[causeIndex] = 3;
      gprs[pcIndex] = specrs[handlerIndex];

      terminalInt = false;
      // fmt::println("TERMINAL INTERRUPT");
      return;
    }
  }
}

// instruction functions

void Proccessor::halt(Instruction inst) {
  if (*((uint32_t *)&inst) != 0) {
    ilInt = true;
    return;
  }
  fmt::println("");
  fmt::println("Emulated processor executed halt instruction");
  fmt::println("Emulated processor state:");
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      fmt::print("r{}={:#x} ", i * 4 + j, gprs[i * 4 + j]);
    }
    fmt::println("");
  }

  exit(0);
}
void Proccessor::softwareInt(Instruction inst) {
  if (inst.a != 0 || inst.b != 0 || inst.c != 0 || inst.d != 0 ||
      inst.mod != 0) {
    ilInt = true;
    return;
  }
  syscallInt = true;
}
void Proccessor::subRoutine(Instruction inst) {
  if (inst.c != 0) {
    ilInt = true;
    return;
  }
  gprs[spIndex] -= 4;
  switch (inst.mod) {
  case 0:
    bus->write(gprs[spIndex], gprs[pcIndex]);
    gprs[pcIndex] = gprs[inst.a] + gprs[inst.b] + inst.d;
    return;
  case 1:
    bus->write(gprs[spIndex], gprs[pcIndex]);
    gprs[pcIndex] = bus->read(gprs[inst.a] + gprs[inst.b] + inst.d);
    return;
  }
}
void Proccessor::jumpOps(Instruction inst) {
  switch (inst.mod) {
  case 0b0000:
    gprs[pcIndex] = gprs[inst.a] + inst.d;
    return;
  case 0b0001:
    if (gprs[inst.b] == gprs[inst.c]) {
      gprs[pcIndex] = gprs[inst.a] + inst.d;
    }
    return;
  case 0b0010:
    if (gprs[inst.b] != gprs[inst.c]) {
      gprs[pcIndex] = gprs[inst.a] + inst.d;
    }
    return;
  case 0b0011: {
    uint64_t b = (int)gprs[inst.b];
    uint64_t c = (int)gprs[inst.c];
    if (b > c) {
      gprs[pcIndex] = gprs[inst.a] + inst.d;
    }
  }
    return;
  case 0b1000:
    gprs[pcIndex] = bus->read(gprs[inst.a] + inst.d);
    return;
  case 0b1001:
    if (gprs[inst.b] == gprs[inst.c]) {
      gprs[pcIndex] = bus->read(gprs[inst.a] + inst.d);
    }
    return;
  case 0b1010:
    if (gprs[inst.b] != gprs[inst.c]) {
      gprs[pcIndex] = bus->read(gprs[inst.a] + inst.d);
    }
    return;
  case 0b1011: {
    uint64_t b = (int)gprs[inst.b];
    uint64_t c = (int)gprs[inst.c];
    if (b > c) {
      gprs[pcIndex] = bus->read(gprs[inst.a] + inst.d);
    }
  }
    return;
  }
}
void Proccessor::atomicSwap(Instruction inst) {
  if (inst.mod != 0 || inst.a != 0 || inst.d != 0) {
    ilInt = true;
    return;
  }
  uint32_t temp = gprs[inst.b];
  gprs[inst.b] = gprs[inst.c];
  gprs[inst.c] = temp;
}
void Proccessor::arithmeticOps(Instruction inst) {
  if (inst.d != 0) {
    ilInt = true;
    return;
  }

  switch (inst.mod) {
  case 0b00:
    gprs[inst.a] = gprs[inst.b] + gprs[inst.c];
    return;
  case 0b01:
    gprs[inst.a] = gprs[inst.b] - gprs[inst.c];
    return;
  case 0b10:
    gprs[inst.a] = gprs[inst.b] * gprs[inst.c];
    return;
  case 0b11:
    gprs[inst.a] = gprs[inst.b] / gprs[inst.c];
    return;
  }
}
void Proccessor::logicOps(Instruction inst) {
  if (inst.d != 0) {
    ilInt = true;
    return;
  }

  switch (inst.mod) {
  case 0b00:
    gprs[inst.a] = ~gprs[inst.b];
    return;
  case 0b01:
    gprs[inst.a] = gprs[inst.b] & gprs[inst.c];
    return;
  case 0b10:
    gprs[inst.a] = gprs[inst.b] | gprs[inst.c];
    return;
  case 0b11:
    gprs[inst.a] = gprs[inst.b] ^ gprs[inst.c];
    return;
  }
}
void Proccessor::bitShift(Instruction inst) {
  if (inst.d != 0) {
    ilInt = true;
    return;
  }

  switch (inst.mod) {
  case 0b0:
    gprs[inst.a] = gprs[inst.b] << gprs[inst.c];
    return;
  case 0b1:
    gprs[inst.a] = gprs[inst.b] >> gprs[inst.c];
    return;
  }
}
void Proccessor::st(Instruction inst) {
  switch (inst.mod) {
  case 0b0:
    bus->write(gprs[inst.a] + gprs[inst.b] + inst.d, gprs[inst.c]);
    return;
  case 0b10: {
    uint32_t adr = bus->read(gprs[inst.a] + gprs[inst.b] + inst.d);
    bus->write(adr, gprs[inst.c]);
    return;
  }
  case 0b1:
    gprs[inst.a] += inst.d;
    bus->write(gprs[inst.a], gprs[inst.c]);
    return;
  }
}
void Proccessor::ld(Instruction inst) {
  uint32_t adr;

  switch (inst.mod) {
  case 0b0:
    gprs[inst.a] = specrs[inst.b];
    return;
  case 0b1:
    gprs[inst.a] = gprs[inst.b] + inst.d;
    return;
  case 0b10:
    adr = gprs[inst.b] + gprs[inst.c] + inst.d;
    gprs[inst.a] = bus->read(gprs[inst.b] + gprs[inst.c] + inst.d);
    return;
  case 0b11:
    gprs[inst.a] = bus->read(gprs[inst.b]);
    gprs[inst.b] += inst.d;
    return;
  case 0b100:
    specrs[inst.a] = gprs[inst.b];
    return;
  case 0b101:
    specrs[inst.a] = specrs[inst.b] | inst.d;
    return;
  case 0b110:
    specrs[inst.a] = bus->read(gprs[inst.b] + gprs[inst.c] + inst.d);
    return;
  case 0b111:
    specrs[inst.a] = bus->read(gprs[inst.b]);
    gprs[inst.b] += inst.d;
    return;
  }
}
