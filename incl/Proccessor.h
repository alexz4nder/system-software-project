#ifndef PROCCESSOR_H
#define PROCCESSOR_H

#include "InstructionProccessing.h"
#include <cstdint>
class Instruction;
class Bus;
class Proccessor {
  Bus *bus;
  Proccessor(Bus *);
  friend class Bus;

  // general purpouse registers
  uint32_t gprs[16];
  static const uint32_t pcIndex = 15;
  static const uint32_t spIndex = 14;

  // special registers
  uint32_t specrs[3];
  static const uint32_t statusIndex = 0;
  static const uint32_t handlerIndex = 1;
  static const uint32_t causeIndex = 2;

  static const uint32_t startAdress = 0x40000000;

  // interrupts
  bool syscallInt = false;
  bool terminalInt = false;
  bool timerInt = false;
  bool ilInt = false; // illgegal instruction

  void doInstruction();
  void ld(Instruction inst);
  void st(Instruction inst);
  void bitShift(Instruction inst);
  void logicOps(Instruction inst);
  void arithmeticOps(Instruction inst);
  void atomicSwap(Instruction inst);
  void jumpOps(Instruction inst);
  void subRoutine(Instruction inst);
  void softwareInt(Instruction inst);
  void halt(Instruction intst);

  void checkAndHandleInterrupt();
};

#endif // !PROCCESSOR_H
