#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include <vector>

enum class SymbolType { UKN, ABS, EXPR, RELOC, EXTERN };
enum class SymbolScope { LOC, GLOB };
enum class SymbolDefined { Defined, Undefined };

struct Symbol {
  std::string name;
  int sectionIndex;
  SymbolDefined defined;
  SymbolScope scope;
  SymbolType type;
  int value;
  std::string expr;
};

Symbol *findSymbol(std::string &name);

extern std::vector<Symbol *> symTable;

std::string format_as(SymbolType type);
std::string format_as(SymbolScope scope);
std::string format_as(SymbolDefined define);
std::string format_as(Symbol symbol);

// auto format_as(SymbolType type);
#endif // !SYMBOL_H
