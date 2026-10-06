#define FMT_HEADER_ONLY
#include "../../incl/BackPatch.h"
#include "../../incl/InstructionProccessing.h"
#include "../../incl/fmt/base.h"
#include "../../incl/fmt/format.h"
#include "../../incl/sections.h"
#include "../../incl/symbol.h"
#include <cstdint>
#include <string>

std::string format_as(SymbolType type) {
  switch (type) {
  case SymbolType::ABS:
    return "ABS";
  case SymbolType::RELOC:
    return "RELOC";
  case SymbolType::UKN:
    return "UKN";
  case SymbolType::EXTERN:
    return "EXTERN";
  case SymbolType::EXPR:
    return "EXPR";
  }
  return "ERROR";
}
std::string format_as(SymbolScope scope) {
  switch (scope) {
  case SymbolScope::GLOB:
    return "GLOB";
  case SymbolScope::LOC:
    return "LOC";
  }
  return "ERROR";
}
std::string format_as(SymbolDefined define) {
  switch (define) {
  case SymbolDefined::Defined:
    return "Defined";
  case SymbolDefined::Undefined:
    return "Undefined";
  }
  return "ERROR";
}
std::string format_as(Symbol symbol) {
  if (symbol.type == SymbolType::EXPR) {
    return fmt::format(
        "NAME:{}|DEF:{}|TYPE:{}|SCOPE:{}|SECTION:{}|VAL:{:#x}|EXPR:{}",
        symbol.name, symbol.defined, symbol.type, symbol.scope,
        symbol.sectionIndex, symbol.value, symbol.expr);
  }
  return fmt::format("NAME:{}|DEF:{}|TYPE:{}|SCOPE:{}|SECTION:{}|VAL:{:#x}",
                     symbol.name, symbol.defined, symbol.type, symbol.scope,
                     symbol.sectionIndex, symbol.value);
}

std::string format_as(BackPatchType bp) {
  switch (bp) {
  case BackPatchType::UnknownSymbol:
    return "UnknownSymbol";
  case BackPatchType::SectionOffset:
    return "SectionOffset";
  case BackPatchType::UnknownDisplacement:
    return "UnknownDisplacement";
  }
  return "ERROR";
}
std::string format_as(BackPatch bp) {
  std::string symbolName = "";
  if (bp.sym != nullptr) {
    symbolName = bp.sym->name;
  }
  return fmt::format("SYM:{}|TYPE:{}|SEC:{}|POS:{:#x}|SectionOffsetIndex:{}",
                     symbolName, bp.type, bp.sectionIndex, bp.position,
                     bp.sectionOffsetIndex);
}

std::string format_as(Section sec) {
  return fmt::format("INDEX:{}|NAME:{}|SIZE:{}", sec.sectionNumber, sec.name,
                     sec.sectionMemory.size());
}

std::string format_as(Instruction inst) {
  return fmt::format("OPCODE:{:#x}|MOD:{:#x}|a:{:#x}|b:{:#x}|c:{:#x}|d:{:#x}",
                     (uint8_t)inst.opcode, (uint8_t)inst.mod, (uint8_t)inst.a,
                     (uint8_t)inst.b, (uint8_t)inst.c, (int)inst.d);
}
