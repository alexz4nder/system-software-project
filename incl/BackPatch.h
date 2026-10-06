#ifndef BACK_PATCH_H
#define BACK_PATCH_H

#include "symbol.h"
#include <string>
#include <vector>
enum class BackPatchType { UnknownSymbol, SectionOffset, UnknownDisplacement };
struct BackPatch {
  BackPatchType type;
  Symbol *sym;
  int sectionIndex;
  int position;
  int sectionOffsetIndex;
};

extern std::vector<BackPatch *> backPatches;

std::string format_as(BackPatchType);
std::string format_as(BackPatch);

#endif // !BACK_PATCH_H
