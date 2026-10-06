#ifndef TOKENS_H
#define TOKENS_H

#include <string>
enum class TokenType { label, directive, instruction };

enum class InstructionName { ld, st, add, push, call, ret };
enum class Register { r0, r1, r2, r3, r4, r5, r6 };
struct Instruction {
  InstructionName name;
  Register reg1;
  Register reg2;
};

enum class DirectiveName {
  error,
  global,   // make symbols global
  external, // add undefined global symbol
  section,
  word,
  skip,
  ascii,
  equ,
  end
};

struct token {
  TokenType type;
  union {};
};

#endif // !TOKENS_H
