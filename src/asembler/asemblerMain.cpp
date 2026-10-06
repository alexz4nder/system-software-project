#define FMT_HEADER_ONLY
#include "../../incl/BackPatch.h"
#include "../../incl/DirectiveProccessing.h"
#include "../../incl/InstructionProccessing.h"
#include "../../incl/ObjFile.h"
#include "../../incl/fmt/base.h"
#include "../../incl/fmt/core.h"
#include "../../incl/fmt/format.h"
#include "../../incl/sections.h"
#include "../../incl/symbol.h"
#include "../../incl/tokenizer.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// GLOBALS
Section *current;
Symbol *lastSymbol = nullptr;
std::vector<Section *> sections;
std::vector<BackPatch *> backPatches;
std::vector<Symbol *> symTable;

Symbol *addLabel(std::string &str);
void fixBackPatches();
void removeLocalSymbols();

int main(int argc, char *argv[]) {
  // input arguments
  bool outIsNext = false;
  char *outputName = nullptr;
  char *inputName = nullptr;
  fmt::println("{}", SymbolType::ABS);
  for (int i = 1; i < argc; i++) {
    if (outIsNext) {
      outputName = argv[i];
      outIsNext = false;
      continue;
    }
    if (strcmp(argv[i], "-o") == 0) {
      outIsNext = true;
      continue;
    }

    if (inputName != nullptr) {
      fmt::println("ERROR:ARGUMENT AFTER INPUT FILE NAME");
      return 0;
    }

    inputName = argv[i];
  }

  if (outputName == nullptr) {
    fmt::println("ERROR:OUTPUT NOT SET");
    return 0;
  }

  fmt::println("inputName:{}\noutputName:{}", inputName, outputName);

  // TRANSLATING
  Tokenizer tokenizer(inputName);
  enum class AssemblerState { empty, label, directive, instruction };
  current = nullptr;

  std::string token;
  while (true) {
    token = tokenizer.getNextToken();
    if (token.empty()) {
      break;
    }
    // fmt::println("TOKEN:{}", token);

    if (token == "\n") {
      continue;
    }

    if (token.at(0) == '.') {
      proccessDirective(token, tokenizer);
    } else if (token.at(token.length() - 1) == ':') {
      lastSymbol = addLabel(token);
      std::string nextToken = tokenizer.getNextToken();
      if (nextToken != "\n") {
        proccessInstruction(tokenizer, nextToken);
      }
    } else {
      proccessInstruction(tokenizer, token);
    }
  }

  // PRINTING SECTIONS
  fmt::println("==========================SECTIONS==========================");
  for (Section *sec : sections) {
    fmt::println("{}", *sec);
  }
  // PRINTING SYMBOLTABLE
  fmt::println(
      "==========================SYMBOL TABLE==========================");
  fmt::println("NUM OF SYMBOLS:{}", symTable.size());
  for (Symbol *sym : symTable) {
    fmt::println("{}", *sym);
  }
  // SECTION DUMPS
  fmt::println(
      "==========================SECTION DUMPS==========================");
  for (Section *sec : sections) {
    fmt::println("SECTION - {}  SIZE - {}", sec->name,
                 sec->sectionMemory.size());
    for (size_t i = 0; i < sec->sectionMemory.size(); i += 16) {
      fmt::print("{:#08x}: ", i);
      for (size_t j = 0; (j < 16 && j + i < sec->sectionMemory.size()); j++) {

        fmt::print("{:02x} ", sec->sectionMemory.at(j + i));
      }
      fmt::println("");
    }
  }

  // BACKPATCHING
  fmt::println(
      "==========================BACKPATCHES==========================");
  for (BackPatch *backPatch : backPatches) {
    fmt::println("{}", *backPatch);
  }

  fixBackPatches();

  // BACKPATCHES AFTER FIXUP
  fmt::println("==========================BACKPATCHES AFRER "
               "FIXUP==========================");
  for (BackPatch *backPatch : backPatches) {
    fmt::println("{}", *backPatch);
  }

  removeLocalSymbols();

  // FINAL SYMBOL TABLE
  fmt::println(
      "==========================FINAL SYMBOL TABLE==========================");
  fmt::println("NUM OF SYMBOLS:{}", symTable.size());
  for (Symbol *sym : symTable) {
    fmt::println("{}", *sym);
  }

  ObjFile objectFileOutput;
  objectFileOutput.symTab = symTable;
  objectFileOutput.backPatches = backPatches;
  objectFileOutput.sections = sections;
  objectFileOutput.dumpToFile(outputName);

  return 0;
}

Symbol *addLabel(std::string &str) {
  if (current == nullptr) {
    fmt::println("ERROR: symbol \"{}\" is outside section", str);
    exit(0);
  }
  str.erase(str.size() - 1, 1);

  // check if alredy exists
  bool exists = false;
  for (Symbol *sym : symTable) {
    if (sym->name == str) {
      if (sym->type != SymbolType::UKN) {
        fmt::println("ERROR: symbol \"{}\" alredy exists", str);
        exit(0);

      } else {
        sym->value = current->sectionMemory.size();
        sym->type = SymbolType::RELOC;
        sym->sectionIndex = current->sectionNumber;
        sym->defined = SymbolDefined::Defined;
        exists = true;
        return sym;
      }
    }
  }
  if (exists == false) {
    Symbol *label = new Symbol();
    label->name = str;
    label->scope = SymbolScope::LOC;
    label->type = SymbolType::RELOC;
    label->value = current->sectionMemory.size();
    label->sectionIndex = sections.size() - 1;
    label->defined = SymbolDefined::Defined;
    symTable.push_back(label);
    return label;
  }
}

void removeLocalSymbols() {
  for (int i = symTable.size() - 1; i >= 0; i--) {
    if (symTable.at(i)->scope == SymbolScope::LOC) {
      symTable.erase(symTable.begin() + i);
    }
  }
}
