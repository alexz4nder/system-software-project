#ifndef SECTIONS_H
#define SECTIONS_H

#include "LitteralPool.h"
#include <cstdint>
#include <string>
#include <vector>

struct PoolBackPatch {
  int instructionAdr;
  int poolTableIndex;
};

class Section {
public:
  int sectionNumber;
  std::string name;
  std::vector<char> sectionMemory;

  int codeStarted = -1;
  static const int maxCodeSize = 0x0800 - 12;
  std::vector<PoolEntry *> poolTable;
  std::vector<PoolBackPatch> poolBackpatch;

  uint32_t offset = 0xffffffff;
  // 0xffffffff for unknown offset

  void insertPoolTable(bool addJump = false);
};

extern std::vector<Section *> sections;

std::string format_as(Section sec);

#endif // SECTIONS_H
