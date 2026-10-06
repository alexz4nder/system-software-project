#ifndef INSTRUCTION_PROCCESSING_H
#define INSTRUCTION_PROCCESSING_H

#include "tokenizer.h"

void proccessInstruction(Tokenizer &tokenizer, std::string &inst);

enum class InstType { NOARG, ONEREG, TWOREG, LD, ST, JMPS, CSRRD, CSRWR };
struct Instruction {

  unsigned int mod : 4;
  unsigned int opcode : 4;
  unsigned int b : 4;
  unsigned int a : 4;
  unsigned int c : 4;
  int d : 12;
  Instruction();
};

/*struct Instruction {
  int d : 12;
  unsigned int c : 4;
  unsigned int b : 4;
  unsigned int a : 4;
  unsigned int mod : 4;
  unsigned int opcode : 4;
  Instruction();
};*/

void insertInstruction(Instruction inst);
std::string format_as(Instruction inst);

#endif // !INSTRUCTION_PROCCESSING_H
