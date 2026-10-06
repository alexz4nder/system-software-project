#define FMT_HEADER_ONLY
#include "../../incl/InstructionProccessing.h"
#include "../../incl/BackPatch.h"
#include "../../incl/fmt/base.h"
#include "../../incl/fmt/core.h"
#include "../../incl/sections.h"
#include "../../incl/symbol.h"
#include "../../incl/tokenizer.h"
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <istream>
#include <sstream>
#include <stack>
#include <string>
#include <sys/types.h>
#include <vector>

extern Section *current;

// utility
int isNumber(std::string &str);
PoolEntry *findPoolEntry(Symbol *sym);
PoolEntry *findPoolEntry(int literal);
void addLiteralToPool(int num);
void addSymbolToPool(std::string symbol);
void insertInt(int data);
int readRegister(std::string &str);
int readSpecReg(std::string &str);

// instruction proccessing
InstType getInstructionType(std::string &inst, Tokenizer &tokenizer);
void addInstruction(int opcode, int a, int b, int c, int d);
void proccessOneReg(Tokenizer &tokenizer, std::string &inst);
void proccessStore(Tokenizer &tokenizer);
void proccessLoad(Tokenizer &tokenizer);
void proccessTwoReg(Tokenizer &tokenizer, std::string &inst);
void proccessJumps(Tokenizer &tokenizer, std::string &inst);
void proccessCsrrd(Tokenizer &tokenizer);
void proccessCstwr(Tokenizer &tokenizer);
void proccessNoarg(Tokenizer &tokenizer, std::string &inst);

void proccessInstruction(Tokenizer &tokenizer, std::string &inst) {

  fmt::println("INST:{}", inst);
  if (current == nullptr) {
  }

  InstType type = getInstructionType(inst, tokenizer);

  switch (type) {
  case InstType::ONEREG:
    proccessOneReg(tokenizer, inst);
    break;
  case InstType::ST:
    proccessStore(tokenizer);
    break;
  case InstType::TWOREG:
    proccessTwoReg(tokenizer, inst);
    break;
  case InstType::JMPS:
    proccessJumps(tokenizer, inst);
    break;
  case InstType::LD:
    proccessLoad(tokenizer);
    break;
  case InstType::CSRWR:
    proccessCstwr(tokenizer);
    break;
  case InstType::CSRRD:
    proccessCsrrd(tokenizer);
    break;
  case InstType::NOARG:
    proccessNoarg(tokenizer, inst);
    break;
  }

  if (current->codeStarted == -1) {
    current->codeStarted = current->sectionMemory.size() - 4;
  } else {
    fmt::println("CODE STARTED:{:#x}",
                 current->codeStarted + Section::maxCodeSize);
    if (current->codeStarted + Section::maxCodeSize - 4 <=
        current->sectionMemory.size()) {
      current->insertPoolTable(true);
    }
  }
}

void proccessStore(Tokenizer &tokenizer) {
  Instruction instruction;
  instruction.opcode = 0b1000;
  instruction.mod = 0b10;

  std::string regString = tokenizer.getNextToken();
  int reg = readRegister(regString);
  instruction.c = reg;

  if (reg == -1) {
    fmt::println(stderr, "[ERROR line:{}] expected register",
                 tokenizer.currentLine);
    exit(0);
  }

  std::string coma = tokenizer.getNextToken();
  if (coma != ",") {
    fmt::println(stderr, "[ERROR line:{}] expected ','", tokenizer.currentLine);
    exit(0);
  }

  std::string nextToken = tokenizer.getNextToken();
  reg = readRegister(nextToken);
  if (reg != -1) {
    // TODO proveriti validnost
    // instruction.b = reg;
  } else if (nextToken == "[") {
    instruction.mod = 0;
    nextToken = tokenizer.getNextToken();
    reg = readRegister(nextToken);
    instruction.a = reg;
    if (reg == -1) {
      fmt::println(stderr, "[ERROR line:{}] expected register inside []",
                   tokenizer.currentLine);
      exit(0);
    }

    nextToken = tokenizer.getNextToken();
    if (nextToken == "+") {
      nextToken = tokenizer.getNextToken();

      int base = isNumber(nextToken);
      if (base != 0) { // literal
        int disp = std::stoi(nextToken, 0, base);
        if (disp < (-((1 << 11) - 1)) || disp >= (((1 << 11) - 1))) {
          fmt::println(stderr, "[ERROR line:{}] displacement out of range",
                       tokenizer.currentLine);
          exit(0);
        }
        instruction.d = disp;
      } else { // symbol
        Symbol *sym = findSymbol(nextToken);
        if (sym == nullptr) {
          sym = new Symbol();
          symTable.push_back(sym);
          sym->defined = SymbolDefined::Undefined;
          sym->type = SymbolType::UKN;
          sym->name = nextToken;
          sym->scope = SymbolScope::LOC;
          sym->sectionIndex = current->sectionNumber;
          sym->value = 0;
        }
        if (sym->defined != SymbolDefined::Defined) {
          BackPatch *bp = new BackPatch();
          bp->sectionIndex = current->sectionNumber;
          bp->sym = sym;
          bp->type = BackPatchType::UnknownDisplacement;
          bp->position = current->sectionMemory.size();
        } else {
          if (sym->type != SymbolType::ABS && sym->type != SymbolType::RELOC &&
              sym->type != SymbolType::EXPR) {
            fmt::println(stderr, "[ERROR line:{}] symbol Type isnt ABS",
                         tokenizer.currentLine);
            exit(0);
          }

          if (sym->type == SymbolType::ABS) {
            int disp = sym->value;
            if (disp < (-((1 << 11) - 1)) || disp >= (((1 << 11) - 1))) {
              fmt::println(stderr, "[ERROR line:{}] displacement out of range",
                           tokenizer.currentLine);
              exit(0);
            }
            instruction.d = disp;
          }
          if (sym->type == SymbolType::EXPR) {
            BackPatch *bp = new BackPatch();
            bp->sectionIndex = current->sectionNumber;
            bp->sym = sym;
            bp->type = BackPatchType::UnknownDisplacement;
            bp->position = current->sectionMemory.size();
          }
          if (sym->type == SymbolType::RELOC) {
            // if (sym->sectionIndex != current->sectionNumber) {
            fmt::println(stderr,
                         "[ERROR line:{}] Proveriti da li ovo ima smisla",
                         tokenizer.currentLine);
            exit(0);
            //}
            // TODO Pitati asistenta da li ovo ima smisla
          }
        }
      }
      nextToken = tokenizer.getNextToken();
      if (nextToken != "]") {
        fmt::println(stderr, "[ERROR line:{}] expected ']' ",
                     tokenizer.currentLine);

        exit(0);
      }
    } else if (nextToken != "]") {
      fmt::println(stderr, "[ERROR line:{}] expected ']' or '+' ",
                   tokenizer.currentLine);

      exit(0);
    }

  } else {
    instruction.a = 15;
    if (nextToken.at(0) == '$') {
      fmt::println(stderr, "[ERROR line:{}] wrong operand");
      exit(0);
    }
    int base = isNumber(nextToken);
    if (base != 0) {
      int num = (int)std::stoul(nextToken, 0, base);
      addLiteralToPool(num);
    } else {
      addSymbolToPool(nextToken);
    }
    instruction.mod = 0b0010;
  }

  nextToken = tokenizer.getNextToken();
  if (nextToken != "\n") {
    fmt::println(stderr, "[ERROR line:{}] expected '\\n' ",
                 tokenizer.currentLine);
    exit(0);
  }

  insertInstruction(instruction);
}

void proccessLoad(Tokenizer &tokenizer) {
  Instruction instruction;
  instruction.opcode = 0b1001;

  bool direct = false;
  bool indirect = false;
  bool regTransfer = false;
  bool pcRel = false;

  std::string nextToken = tokenizer.getNextToken();
  int reg = readRegister(nextToken);
  if (reg != -1) {
    instruction.b = reg;
    regTransfer = true;
  } else if (nextToken == "[") {
    direct = true;
    nextToken = tokenizer.getNextToken();
    reg = readRegister(nextToken);
    instruction.b = reg;
    if (reg == -1) {
      fmt::println(stderr, "[ERROR line:{}] expected register inside []",
                   tokenizer.currentLine);
      exit(0);
    }

    nextToken = tokenizer.getNextToken();
    if (nextToken == "+") {
      nextToken = tokenizer.getNextToken();

      int base = isNumber(nextToken);
      if (base != 0) { // literal
        int disp = std::stoi(nextToken, 0, base);
        if (disp < (-((1 << 11) - 1)) || disp >= (((1 << 11) - 1))) {
          fmt::println(stderr, "[ERROR line:{}] displacement out of range",
                       tokenizer.currentLine);
          exit(0);
        }
        instruction.d = disp;
      } else { // symbol
        Symbol *sym = findSymbol(nextToken);
        if (sym == nullptr) {
          sym = new Symbol();
          symTable.push_back(sym);
          sym->defined = SymbolDefined::Undefined;
          sym->type = SymbolType::UKN;
          sym->name = nextToken;
          sym->scope = SymbolScope::LOC;
          sym->sectionIndex = current->sectionNumber;
          sym->value = 0;
        }
        if (sym->defined != SymbolDefined::Defined) {
          BackPatch *bp = new BackPatch();
          bp->sectionIndex = current->sectionNumber;
          bp->sym = sym;
          bp->type = BackPatchType::UnknownDisplacement;
          bp->position = current->sectionMemory.size();
        } else {
          if (sym->type != SymbolType::ABS && sym->type != SymbolType::RELOC &&
              sym->type != SymbolType::EXPR) {
            fmt::println(stderr, "[ERROR line:{}] symbol Type isnt ABS",
                         tokenizer.currentLine);
            exit(0);
          }

          if (sym->type == SymbolType::ABS) {
            int disp = sym->value;
            if (disp < (-((1 << 11) - 1)) || disp >= (((1 << 11) - 1))) {
              fmt::println(stderr, "[ERROR line:{}] displacement out of range",
                           tokenizer.currentLine);
              exit(0);
            }
            instruction.d = disp;
          }
          if (sym->type == SymbolType::EXPR) {
            BackPatch *bp = new BackPatch();
            bp->sectionIndex = current->sectionNumber;
            bp->sym = sym;
            bp->type = BackPatchType::UnknownDisplacement;
            bp->position = current->sectionMemory.size();
          }
          if (sym->type == SymbolType::RELOC) {
            // if (sym->sectionIndex != current->sectionNumber) {
            fmt::println(
                stderr,
                "[ERROR line:{}] displacement not known at assembly time",
                tokenizer.currentLine);
            exit(0);
            //}
            // TODO Pitati asistenta da li ovo ima smisla
          }
        }
      }
      nextToken = tokenizer.getNextToken();
      if (nextToken != "]") {
        fmt::println(stderr, "[ERROR line:{}] expected ']' ",
                     tokenizer.currentLine);

        exit(0);
      }
    } else if (nextToken != "]") {
      fmt::println(stderr, "[ERROR line:{}] expected ']' or '+' ",
                   tokenizer.currentLine);

      exit(0);
    }

  } else {
    if (nextToken.at(0) != '$') {
      indirect = true;
    } else {
      direct = true;
      nextToken.erase(0, 1);
    }
    pcRel = true;
    int base = isNumber(nextToken);
    if (base != 0) {
      int num = (int)std::stoul(nextToken, 0, base);
      addLiteralToPool(num);
    } else {
      addSymbolToPool(nextToken);
    }
  }

  std::string coma = tokenizer.getNextToken();
  if (coma != ",") {
    fmt::println(stderr, "[ERROR line:{}] expected ',' ",
                 tokenizer.currentLine);
    exit(0);
  }
  std::string dstReg = tokenizer.getNextToken();
  reg = readRegister(dstReg);
  if (reg == -1) {
    fmt::println(stderr, "[ERROR line:{}] expected register ",
                 tokenizer.currentLine);
    exit(0);
  }
  instruction.a = reg;
  std::string endline = tokenizer.getNextToken();
  if (endline != "\n") {
    fmt::println(stderr, "[ERROR line:{}] expected new line ",
                 tokenizer.currentLine);
    exit(0);
  }

  if (regTransfer) {
    instruction.mod = 0b0001;
  } else if (indirect) {
    instruction.mod = 0b0010;
    instruction.b = 15;
    insertInstruction(instruction);
    pcRel = false;
    instruction.b = instruction.a;
  } else if (direct) {
    instruction.mod = 0b0010;
  }

  if (pcRel) {
    instruction.b = 15;
  }

  insertInstruction(instruction);
}

void proccessTwoReg(Tokenizer &tokenizer, std::string &inst) {
  std::string srcString = tokenizer.getNextToken();
  int srcRegIndex = readRegister(srcString);
  if (srcRegIndex == -1) {
    fmt::println(stderr, "[ERROR line:{}]: expected register in \"{}\"",
                 tokenizer.currentLine, inst);
    exit(0);
  }

  // check ','
  std::string coma = tokenizer.getNextToken();
  if (coma != ",") {
    fmt::println(stderr, "[ERROR line:{}]: expected ',' in \"{}\"",
                 tokenizer.currentLine, inst);
    exit(0);
  }

  std::string dstString = tokenizer.getNextToken();
  int dstRegIndex = readRegister(dstString);
  if (dstRegIndex == -1) {
    fmt::println(stderr, "[ERROR line:{}]: expected register in \"{}\"",
                 tokenizer.currentLine, inst);
    exit(0);
  }

  // check '\n'
  std::string newLine = tokenizer.getNextToken();
  if (newLine != "\n") {
    fmt::println(stderr, "[ERROR line:{}]: expected new line in \"{}\"",
                 tokenizer.currentLine, inst);
    exit(0);
  }

  std::vector<char> &mem = current->sectionMemory;
  Instruction instruction;

  if (inst == "add") {
    instruction.opcode = 0b0101;
    instruction.mod = 0;
  } else if (inst == "sub") {
    instruction.opcode = 0b0101;
    instruction.mod = 1;
  } else if (inst == "mul") {
    instruction.opcode = 0b0101;
    instruction.mod = 0b10;
  } else if (inst == "div") {
    instruction.opcode = 0b0101;
    instruction.mod = 0b11;
  } else if (inst == "and") {
    instruction.opcode = 0b0110;
    instruction.mod = 1;
  } else if (inst == "or") {
    instruction.opcode = 0b0110;
    instruction.mod = 0b10;
  } else if (inst == "xor") {
    instruction.opcode = 0b0110;
    instruction.mod = 0b11;
  } else if (inst == "shl") {
    instruction.opcode = 0b0111;
    instruction.mod = 0;
  } else if (inst == "shr") {
    instruction.opcode = 0b0111;
    instruction.mod = 1;
  } else if (inst == "xchg") {
    instruction.opcode = 0b0100;
    instruction.mod = 0;
    instruction.b = srcRegIndex;
    instruction.c = dstRegIndex;
    insertInstruction(instruction);
    return;
  }
  instruction.a = dstRegIndex;
  instruction.b = dstRegIndex;
  instruction.c = srcRegIndex;

  insertInstruction(instruction);
}

void proccessOneReg(Tokenizer &tokenizer, std::string &inst) {
  std::vector<char> &mem = current->sectionMemory;
  Instruction instruction;

  std::string arg = tokenizer.getNextToken();
  int index = readRegister(arg);
  if (index == -1) {
    fmt::println(stderr, "[ERROR line:{}]: expected register",
                 tokenizer.currentLine);
    exit(0);
  }

  if (inst == "push") {
    instruction.opcode = 0b1000;
    instruction.mod = 0b0001;
    instruction.c = index;
    instruction.a = 0xe;
    instruction.d = -4;
  }
  if (inst == "pop") {
    instruction.opcode = 0b1001;
    instruction.mod = 0b0011;
    instruction.a = index;
    instruction.b = 0xe;
    instruction.d = 4;
  }
  if (inst == "not") {
    instruction.opcode = 0b0110;
    instruction.a = index;
    instruction.b = index;
  }

  std::string endl = tokenizer.getNextToken();
  if (endl != "\n") {
    fmt::println(stderr,
                 "[ERROR line:{}] to many arguments for \"{}\" instruction",
                 tokenizer.currentLine, inst);
    exit(0);
  }

  insertInstruction(instruction);
}

void proccessNoarg(Tokenizer &tokenizer, std::string &inst) {
  Instruction instruction;
  if (inst == "int") {
    instruction.opcode = 1;
  }
  if (inst == "ret") {
    instruction.opcode = 0b1001;
    instruction.mod = 0b0011;
    instruction.a = 0xf;
    instruction.b = 0xe;
    instruction.d = 4;
  }
  if (inst == "iret") {
    instruction.opcode = 0b1001;
    instruction.mod = 0b0110;
    instruction.a = 0;
    instruction.b = 14;
    instruction.d = 1;
    insertInstruction(instruction);

    // TODO add check if instruction max size is reached
    // and insert literal pool if neccessary

    instruction.opcode = 0b1001;
    instruction.mod = 0b0011;
    instruction.a = 15;
    instruction.b = 14;
    instruction.d = 2;
  }

  insertInstruction(instruction);
}

void proccessJumps(Tokenizer &tokenizer, std::string &inst) {
  Instruction instruction;
  instruction.opcode = 0b0011;
  instruction.a = 15;
  if (inst != "jmp" && inst != "call") {
    std::string nextToken = tokenizer.getNextToken();
    int b = readRegister(nextToken);
    if (b == -1) {
      fmt::println(stderr, "[ERROR line:{}]Expected register in {}",
                   tokenizer.currentLine, inst);
      exit(0);
    }
    instruction.b = b;
    // check ','
    nextToken = tokenizer.getNextToken();
    if (nextToken != ",") {
      fmt::println(stderr, "[ERROR line:{}]Expected ',' in {}",
                   tokenizer.currentLine, inst);
      exit(0);
    }

    nextToken = tokenizer.getNextToken();
    int c = readRegister(nextToken);
    if (c == -1) {
      fmt::println(stderr, "[ERROR line:{}]Expected register in {}",
                   tokenizer.currentLine, inst);
      exit(0);
    }
    instruction.c = c;

    // check ','
    nextToken = tokenizer.getNextToken();
    if (nextToken != ",") {
      fmt::println(stderr, "[ERROR line:{}]Expected ',' in {}",
                   tokenizer.currentLine, inst);
      exit(0);
    }
  }

  std::string operand = tokenizer.getNextToken();
  int base = isNumber(operand);
  if (base != 0) {
    int num = (int)std::stoul(operand, 0, base);
    addLiteralToPool(num);
  } else {
    addSymbolToPool(operand);
  }

  if (inst == "jmp") {
    instruction.mod = 0b1000;
  } else if (inst == "beq") {
    instruction.mod = 0b1001;
  } else if (inst == "bne") {
    instruction.mod = 0b1010;
  } else if (inst == "bgt") {
    instruction.mod = 0b1011;
  } else if (inst == "call") {
    instruction.opcode = 0b0010;
    instruction.mod = 0b0001;
  }
  insertInstruction(instruction);
}

void proccessCsrrd(Tokenizer &tokenizer) {
  std::string nextToken = tokenizer.getNextToken();
  int specReg = readSpecReg(nextToken);

  if (specReg == -1) {
    fmt::println(stderr, "[ERROR line:{}] expected special register",
                 tokenizer.currentLine);
    exit(0);
  }

  nextToken = tokenizer.getNextToken();
  if (nextToken != ",") {
    fmt::println(stderr, "[ERROR line:{}] expected ','", tokenizer.currentLine);
    exit(0);
  }

  nextToken = tokenizer.getNextToken();
  int reg = readRegister(nextToken);
  if (reg == -1) {
    fmt::println(stderr, "[ERROR line:{}] expected register",
                 tokenizer.currentLine);
    exit(0);
  }

  nextToken = tokenizer.getNextToken();
  if (nextToken != "\n") {
    fmt::println(stderr, "[ERROR line:{}] expected new line",
                 tokenizer.currentLine);
    exit(0);
  }

  Instruction instruction;
  instruction.opcode = 0b1001;
  instruction.mod = 0;
  instruction.a = reg;
  instruction.b = specReg;
  insertInstruction(instruction);
}
void proccessCstwr(Tokenizer &tokenizer) {
  std::string nextToken = tokenizer.getNextToken();
  int reg = readRegister(nextToken);

  if (reg == -1) {
    fmt::println(stderr, "[ERROR line:{}] expected register",
                 tokenizer.currentLine);
    exit(0);
  }

  nextToken = tokenizer.getNextToken();
  if (nextToken != ",") {
    fmt::println(stderr, "[ERROR line:{}] expected ','", tokenizer.currentLine);
    exit(0);
  }

  nextToken = tokenizer.getNextToken();
  int specReg = readSpecReg(nextToken);
  if (specReg == -1) {
    fmt::println(stderr, "[ERROR line:{}] expected special register",
                 tokenizer.currentLine);
    exit(0);
  }

  nextToken = tokenizer.getNextToken();
  if (nextToken != "\n") {
    fmt::println(stderr, "[ERROR line:{}] expected new line",
                 tokenizer.currentLine);
    exit(0);
  }

  Instruction instruction;
  instruction.opcode = 0b1001;
  instruction.mod = 0b0100;
  instruction.a = specReg;
  instruction.b = reg;
  insertInstruction(instruction);
}

std::vector<std::string> twoReg = {"xchg", "add", "sub", "mul", "div",
                                   "and",  "or",  "xor", "shl", "shr"};
std::vector<std::string> oneReg = {"push", "pop", "not"};
std::vector<std::string> jumps = {"jmp", "beq", "bne", "bgt", "call"};
std::vector<std::string> noaArg = {"halt", "int", "iret", "ret"};
bool existsInVector(std::vector<std::string> &vec, std::string &str) {
  for (std::string &element : vec) {
    if (element == str) {
      return true;
    }
  }

  return false;
}
InstType getInstructionType(std::string &inst, Tokenizer &tokenizer) {
  if (existsInVector(twoReg, inst)) {
    return InstType::TWOREG;
  }
  if (existsInVector(oneReg, inst)) {
    return InstType::ONEREG;
  }
  if (inst == "st") {
    return InstType::ST;
  }
  if (existsInVector(jumps, inst)) {
    return InstType::JMPS;
  }
  if (inst == "ld") {
    return InstType::LD;
  }
  if (inst == "csrrd") {
    return InstType::CSRRD;
  }
  if (inst == "csrwr") {
    return InstType::CSRWR;
  }
  if (existsInVector(noaArg, inst)) {
    return InstType::NOARG;
  }

  fmt::println(stderr, "[ERROR line:{}] instruction \"{}\" does not exist",
               tokenizer.currentLine, inst);
  exit(0);
}

int readRegister(std::string &str) {
  if (str == "%sp") {
    return 14;
  }
  if (str == "%pc") {
    return 15;
  }
  if (str[0] != '%' || str[1] != 'r') {
    return -1;
  }
  str.erase(0, 2);
  unsigned int index = -1;

  try {
    index = std::stoul(str);
  } catch (std::exception) {
    return -1;
  }
  if (index > 15) {
    return -1;
  }
  return index;
}

Instruction::Instruction() { *((int *)this) = 0; }

void insertInt(int data) {
  std::vector<char> &mem = current->sectionMemory;
  char *ip = (char *)(&data);
  mem.push_back(*(ip++));
  mem.push_back(*(ip++));
  mem.push_back(*(ip++));
  mem.push_back(*(ip));
}

void insertInstruction(Instruction inst) {
  std::vector<char> &mem = current->sectionMemory;
  char *ip = (char *)(&inst);
  mem.push_back(*(ip++));
  mem.push_back(*(ip++));
  mem.push_back(*(ip++));
  mem.push_back(*(ip));
}

Symbol *findSymbol(std::string &name) {
  for (Symbol *sym : symTable) {
    if (sym->name == name) {
      return sym;
    }
  }
  return nullptr;
}

PoolEntry *findPoolEntry(Symbol *sym) {
  for (PoolEntry *poolEntry : current->poolTable) {
    if (poolEntry->sym == sym) {
      return poolEntry;
    }
  }
  return nullptr;
}
PoolEntry *findPoolEntry(int literal) {
  for (PoolEntry *poolEntry : current->poolTable) {
    if (poolEntry->type == PoolType::literal && poolEntry->value == literal) {
      return poolEntry;
    }
  }
  return nullptr;
}

void Section::insertPoolTable(bool addJump) {
  int codeEnd = sectionMemory.size();
  fmt::println("INSERTIG POOL TABLE");

  // TODO insert jump
  if (addJump) {
    Instruction inst;
    inst.opcode = 0b0011;
    inst.a = 15;
    inst.d = poolTable.size() * 4;
    insertInstruction(inst);
    codeEnd += 4;
  }

  fmt::println(stderr, "======================");
  for (int i = 0; i < poolTable.size(); i++) {
    PoolEntry *pe = poolTable.at(i);
    if (pe->type == PoolType::symbol) {
      fmt::println(stderr, "POOL:{} {}", i, pe->sym->name);
    }
    if (pe->type == PoolType::literal) {
      fmt::println(stderr, "POOL:{} {:#x}", i, pe->value);
    }
  }

  for (size_t i = 0; i < poolTable.size(); i++) {
    PoolEntry *pe = poolTable.at(i);
    if (pe->type == PoolType::literal) {
      insertInt(pe->value);
      fmt::println(stderr, "INSERTING LITTERAL:{}", pe->value);
    } else if (pe->type == PoolType::symbol) {
      Symbol *sym = pe->sym;
      fmt::println(stderr, "POOL PATCHING:{}", *sym);
      if (sym->defined == SymbolDefined::Undefined) {
        BackPatch *bp = new BackPatch();
        bp->sym = pe->sym;
        bp->sectionIndex = current->sectionNumber;
        bp->position = current->sectionMemory.size();
        bp->type = BackPatchType::UnknownSymbol;
        insertInt(0);
        fmt::println(stderr, "INSERTING UNKNOWN SYMBOL:{}", *sym);
        backPatches.push_back(bp);

      } else { // symbol defined
        if (sym->type == SymbolType::ABS) {
          insertInt(sym->value);
        }
        if (sym->type == SymbolType::RELOC) {
          BackPatch *bp = new BackPatch();
          bp->sym = pe->sym;
          bp->sectionOffsetIndex = bp->sym->sectionIndex;
          bp->sectionIndex = current->sectionNumber;
          bp->position = current->sectionMemory.size();
          bp->type = BackPatchType::SectionOffset;
          insertInt(sym->value);
          fmt::println(stderr, "INSERTING RELOC SYMBOL:{}", *sym);
          backPatches.push_back(bp);
        }
        if (sym->type == SymbolType::EXTERN || sym->type == SymbolType::EXPR) {
          BackPatch *bp = new BackPatch();
          bp->sym = pe->sym;
          bp->sectionIndex = current->sectionNumber;
          bp->position = current->sectionMemory.size();
          bp->type = BackPatchType::UnknownSymbol;
          insertInt(0);
          fmt::println(stderr, "INSERTING EXPR OR ExTERN:{}", *sym);
          backPatches.push_back(bp);
        }
        if (sym->type == SymbolType::UKN) {
          BackPatch *bp = new BackPatch();
          bp->sym = pe->sym;
          bp->sectionIndex = current->sectionNumber;
          bp->position = current->sectionMemory.size();
          bp->type = BackPatchType::UnknownSymbol;
          insertInt(0);
          backPatches.push_back(bp);
        }
      }
    }
  }

  // Patching in offsets

  fmt::println("CODE END:{:#x}", codeEnd);
  for (PoolBackPatch bp : poolBackpatch) {
    Instruction *ins = (Instruction *)&sectionMemory.at(bp.instructionAdr);
    int displacement = codeEnd + 4 * bp.poolTableIndex - bp.instructionAdr - 4;
    ins->d = displacement;
  }
  poolBackpatch.clear();

  codeStarted = -1;
  poolTable.clear();
}

void addLiteralToPool(int num) {
  PoolEntry *pe = findPoolEntry(num);
  if (pe == nullptr) {
    pe = new PoolEntry();
    pe->type = PoolType::literal;
    pe->value = num;
    pe->index = current->poolTable.size();
    current->poolTable.push_back(pe);
  }
  // creating poolBackpatch
  PoolBackPatch bp;
  bp.poolTableIndex = pe->index;
  bp.instructionAdr = current->sectionMemory.size();
  current->poolBackpatch.push_back(bp);
}
void addSymbolToPool(std::string symbol) {
  Symbol *sym = findSymbol(symbol);
  PoolEntry *pe = nullptr;
  if (sym == nullptr) {
    sym = new Symbol();
    symTable.push_back(sym);
    sym->defined = SymbolDefined::Undefined;
    sym->type = SymbolType::UKN;
    sym->name = symbol;
    sym->scope = SymbolScope::LOC;
    sym->sectionIndex = current->sectionNumber;
    sym->value = 0;
  } else {
    pe = findPoolEntry(sym);
  }

  if (pe == nullptr) {
    pe = new PoolEntry();
    pe->type = PoolType::symbol;
    pe->sym = sym;
    pe->index = current->poolTable.size();
    current->poolTable.push_back(pe);
  }

  // creating poolBackpatch
  PoolBackPatch bp;
  bp.poolTableIndex = pe->index;
  bp.instructionAdr = current->sectionMemory.size();
  current->poolBackpatch.push_back(bp);
}
int readSpecReg(std::string &str) {
  if (str == "%status") {
    return 0;
  }
  if (str == "%handler") {
    return 1;
  }
  if (str == "%cause") {
    return 2;
  }
  return -1;
}

struct StackSymbolEntry {
  Symbol *sym;
  int coeff;
  int sec = 0;
  int add = 0;
};
struct StackEntry {
  int numberPart;
  std::vector<StackSymbolEntry> syms;
};
void pushSym(std::stack<StackEntry> &st, Symbol *sym) {
  static int externalSec = 0x7fffffff;
  StackEntry se;
  se.numberPart = 0;
  StackSymbolEntry sse{.sym = sym, .coeff = 1};
  if (sym->type == SymbolType::EXTERN) {
    sse.sec = externalSec--;
  } else {
    if (sym->type == SymbolType::ABS) {
      sse.sec = -1;
    } else {
      sse.sec = sym->sectionIndex;
    }
    sse.add = sym->value;
  }
  se.syms.push_back(sse);
  st.push(se);
}
void pushNum(std::stack<StackEntry> &st, int num) {
  StackEntry se;
  se.numberPart = num;
  st.push(se);
}
void stackAdd(std::stack<StackEntry> &st) {
  StackEntry se = st.top();
  st.pop();
  StackEntry &top = st.top();

  for (int i = 0; i < se.syms.size(); i++) {
    StackSymbolEntry &symE = se.syms.at(i);
    bool found = false;
    for (int j = 0; j < top.syms.size(); j++) {
      StackSymbolEntry &symETop = top.syms.at(j);
      if (symETop.sec == symE.sec) {
        found = true;

        symETop.add += symE.add;
        symETop.coeff += symE.coeff;

        if (symETop.coeff == 0) {
          top.numberPart += symETop.add;
          top.syms.erase(top.syms.begin() + j);
        }
        break;
      }
    }
    if (found == false) {
      top.syms.push_back(symE);
    }
  }

  top.numberPart += se.numberPart;
}
void stackSub(std::stack<StackEntry> &st) {
  StackEntry se = st.top();
  st.pop();
  StackEntry &top = st.top();

  for (int i = 0; i < se.syms.size(); i++) {
    StackSymbolEntry &symE = se.syms.at(i);
    bool found = false;
    for (int j = top.syms.size() - 1; j >= 0; j++) {
      StackSymbolEntry &symETop = top.syms.at(j);
      if (symETop.sec == symE.sec) {
        found = true;

        symETop.add -= symE.add;
        symETop.coeff -= symE.coeff;

        if (symETop.coeff == 0) {
          top.numberPart += symETop.add;
          top.syms.erase(top.syms.begin() + j);
        }

        break;
      }
    }

    if (found == false) {
      top.syms.push_back(symE);
    }
  }

  top.numberPart -= se.numberPart;
}

bool resolveExpr(Symbol *sym) {
  std::stringstream stream(sym->expr);
  std::string nextToken;
  fmt::println("RESOLVING EXPRESSION:{}", sym->expr);

  std::stack<StackEntry> st;

  int value = 0;

  fmt::println("SYMNAME:{}", sym->name);

  while (!stream.eof()) {
    stream >> nextToken;
    fmt::println("{}", nextToken);
    if (nextToken == "-") {
      stackSub(st);
      continue;
    }
    if (nextToken == "+") {
      stackAdd(st);
      continue;
    }
    if (nextToken == "*") {
      continue;
    }
    if (nextToken == "/") {
      continue;
    }

    int base = isNumber(nextToken);
    if (base != 0) {
      int num;
      if (base == 16) {
        num = std::stoul(nextToken, 0, base);
      } else {
        num = std::stoi(nextToken, 0, base);
      }
      pushNum(st, num);
    } else {
      Symbol *sym = findSymbol(nextToken);
      if (sym == nullptr) {
        fmt::println(stderr, "[ERROR] Unknown symbol \"{}\" ", nextToken);
        exit(0);
      }
      if (sym->type == SymbolType::EXPR) {
        return false;
      }
      if (sym->type == SymbolType::ABS) {
        pushNum(st, sym->value);
      } else {
        pushSym(st, sym);
      }
    }
  }

  if (st.size() != 1) {
    fmt::println(stderr, "[ERROR] error resolving expression \"{}\" ",
                 sym->expr);
    exit(0);
  }
  if (st.top().syms.size() > 1) {
    fmt::println(
        stderr,
        "[ERROR] Expression not relocatable with more than 1 symbol \"{}\" ",
        sym->expr);
    exit(0);
  }

  sym->value = st.top().numberPart;
  if (st.top().syms.size() == 0) {
    sym->type = SymbolType::ABS;
  } else {
    sym->type = st.top().syms.at(0).sym->type;
    int sec = -1;
    if (st.top().syms.at(0).sec >= sections.size()) {
      sec = st.top().syms.at(0).sec;
    }
    sym->sectionIndex = sec;
  }
  sym->defined = SymbolDefined::Defined;

  fmt::print("EXPR num:{:#x}|", st.top().numberPart);
  for (int i = 0; i < st.top().syms.size(); i++) {
    StackSymbolEntry &se = st.top().syms.at(i);
    fmt::println("sec:{} coeff:{} add:{} |", se.sec, se.coeff, se.add);
  }
  fmt::println("");
  return true;
}
void fixBackPatches() {
  fmt::println("FIXING BACKPATCHES");

  /*for (Symbol *sym : symTable) {
    if (sym->type == SymbolType::EXPR) {
      resolveExpr(sym);
    }
  }*/
  while (true) {
    bool exprFound = false;
    bool exprFixed = false;
    for (Symbol *sym : symTable) {
      if (sym->type == SymbolType::EXPR) {
        exprFound = true;
        exprFixed = exprFixed || resolveExpr(sym);
      }
    }

    if (exprFound == false) {
      break;
    }
    if (exprFixed == false) {
      fmt::println(stderr, "[ERROR] cantr resolve expressions");
      exit(0);
    }
  }

  for (int i = backPatches.size() - 1; i >= 0; i--) {
    BackPatch *bp = backPatches.at(i);
    if (bp->type == BackPatchType::SectionOffset) {
      bp->sectionOffsetIndex = bp->sym->sectionIndex;
      bp->sym = nullptr;
      continue;
    }
    if (bp->type == BackPatchType::UnknownSymbol) {
      if (bp->sym->type == SymbolType::EXTERN) {
        continue;
      }
      if (bp->sym->defined == SymbolDefined::Undefined) {
        fmt::println(stderr, "[ERROR] symbol \"{}\" is undefined",
                     bp->sym->name);
        exit(0);
      }

      Section *sec = sections.at(bp->sectionIndex);
      *((int *)(&sec->sectionMemory.at(bp->position))) = bp->sym->value;
      if (bp->sym->type == SymbolType::RELOC) {
        BackPatch *reloc = new BackPatch();
        reloc->type = BackPatchType::SectionOffset;
        reloc->sectionIndex = sec->sectionNumber;
        reloc->position = bp->position;
        reloc->sectionOffsetIndex = bp->sym->sectionIndex;
        backPatches.push_back(reloc);
      }

      backPatches.erase(backPatches.begin() + i);
      continue;
    }
    if (bp->type == BackPatchType::UnknownDisplacement) {
      if (bp->sym->defined == SymbolDefined::Undefined) {
        fmt::println(stderr,
                     "[ERROR] symbol \"{}\" for displacement must be known at "
                     "assembly time ",
                     bp->sym->name);
        exit(0);
      }

      if (bp->sym->type == SymbolType::RELOC) {
        fmt::println(stderr, "[ERROR] dispacement symbol \"{}\" is RELOC",
                     bp->sym->name);
        exit(0);
      }

      Section *sec = sections.at(bp->sectionIndex);
      Instruction *instruction =
          (Instruction *)(&sec->sectionMemory.at(bp->position));
      int disp = bp->sym->value;
      if (disp < (-((1 << 11) - 1)) || disp >= (((1 << 11) - 1))) {
        fmt::println(stderr, "[ERROR] displacement symbol \"{}\" out of range",
                     bp->sym->name);
        exit(0);
      }
      instruction->d = bp->sym->value;
    }
  }
}
