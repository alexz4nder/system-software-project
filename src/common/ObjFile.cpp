#define FMT_HEADER_ONLY
#include "../../incl/ObjFile.h"
#include "../../incl/BackPatch.h"
#include "../../incl/fmt/base.h"
#include "../../incl/fmt/core.h"
#include "../../incl/sections.h"
#include "../../incl/symbol.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <string>

ObjFile::ObjFile() {}
ObjFile::ObjFile(char *filename) {
  this->name = filename;
  std::fstream input(filename, std::ios::in);
  
  while (!input.eof()) {
    std::string type;
    input >> type;
    if (type == "sym") {
      parseSymbol(input);
    } else if (type == "bp") {
      parseBackPatch(input);
    } else if (type == "sec") {
      parseSection(input);
    }
  }
}

void ObjFile::parseSymbol(std::fstream &input) {
  Symbol *sym = new Symbol();
  input >> sym->name >> sym->sectionIndex;

  std::string defined;
  input >> defined;
  if (defined == "Defined") {
    sym->defined == SymbolDefined::Defined;
  } else if (defined == "Undefined") {
    sym->defined = SymbolDefined::Undefined;
  }

  std::string symType;
  input >> symType;
  if (symType == "ABS") {
    sym->type = SymbolType::ABS;
  } else if (symType == "UKN") {
    sym->type = SymbolType::UKN;
  } else if (symType == "EXTERN") {
    sym->type = SymbolType::EXTERN;
  } else if (symType == "RELOC") {
    sym->type = SymbolType::RELOC;
  }

  input >> sym->value;
  symTab.push_back(sym);
}

void ObjFile::parseBackPatch(std::fstream &input) {
  BackPatch *bp = new BackPatch();

  std::string type;
  input >> type;
  if (type == "SectionOffset") {
    bp->type = BackPatchType::SectionOffset;
  } else if (type == "UnknownSymbol") {
    bp->type == BackPatchType::UnknownSymbol;
  }

  input >> bp->sectionIndex;
  input >> bp->position;
  std::string symbolName;
  input >> symbolName;

  if (symbolName == "#") {
    bp->sym = nullptr;
  } else {
    for (Symbol *sym : symTab) {
      if (sym->name == symbolName) {
        bp->sym = sym;
      }
    }
  }

  input >> bp->sectionOffsetIndex;

  backPatches.push_back(bp);
}

void ObjFile::parseSection(std::fstream &input) {
  Section *sec = new Section();

  input >> sec->name;
  int secSize;
  input >> secSize;

  for (int i = 0; i < secSize; i++) {
    std::string byteStr;
    input >> byteStr;
    int toInt = std::stoi(byteStr, 0, 16);
    char byte = 0 | toInt;

    sec->sectionMemory.push_back(byte);
  }

  sec->sectionNumber = sections.size();
  sections.push_back(sec);
}

void ObjFile::dumpToFile(char *outputName) {
  FILE *f = fopen(outputName, "w");

  if (f == nullptr) {
    fmt::println(stderr, "[ERROR] file creation error");
    exit(0);
  }

  for (int i = 0; i < symTab.size(); i++) {
    Symbol *sym = symTab.at(i);
    fmt::println(f, "sym {} {} {} {} {}", sym->name, sym->sectionIndex,
                 sym->defined, sym->type, sym->value);
  }

  for (int i = 0; i < backPatches.size(); i++) {
    BackPatch *bp = backPatches.at(i);

    std::string symname = "#"; // # indicates no symbol
    if (bp->sym != nullptr) {
      symname = bp->sym->name;
    }

    fmt::println(f, "bp {} {} {} {} {}", bp->type, bp->sectionIndex,
                 bp->position, symname, bp->sectionOffsetIndex);
  }

  for (int i = 0; i < sections.size(); i++) {
    Section *sec = sections.at(i);

    fmt::println(f, "sec {} {}", sec->name, sec->sectionMemory.size());

    for (int i = 0; i < sec->sectionMemory.size(); i++) {

      fmt::print(f, "{:02x} ", sec->sectionMemory.at(i));

      if (i % 4 == 3 || i == sec->sectionMemory.size() - 1) {
        fmt::println(f, "");
      }
    }
  }

  fclose(f);
}

void ObjFile::mergeWithObjFile(ObjFile *file) {
  int sectionsInFirstFile = sections.size();
  int backPatchesInFirstFile = backPatches.size();

  for (int i = 0; i < sections.size(); i++) {
    fmt::println(stderr, "{}", *sections.at(i));
  }
  for (int i = 0; i < backPatches.size(); i++) {
    fmt::println(stderr, "{}", *backPatches.at(i));
  }
  fmt::println(stderr, "========PRE======================");

  for (int i = 0; i < sectionsInFirstFile; i++) {
    Section *sec1 = sections.at(i);
    for (int j = 0; j < file->sections.size(); j++) {
      Section *sec2 = file->sections.at(j);
      if (sec1->name == sec2->name) {
        mergeSections(i, file, j);
        break;
      }
    }
  }

  // add remaining sections
  for (int i = 0; i < file->sections.size(); i++) {
    Section *sec2 = file->sections.at(i);
    if (sec2->sectionNumber != -1) {
      sec2->sectionNumber = sections.size();
      sections.push_back(sec2);
    }
  }
  for (int i = 0; i < file->sections.size(); i++) {
    Section *section = file->sections.at(i);
    if (section->sectionNumber == -1) {
      for (Section *s : sections) {
        if (s->name == section->name) {
          section->sectionNumber = s->sectionNumber;
        }
      }
    }
  }

  // add remaining symbols
  for (Symbol *sym : file->symTab) {
    if (sym->sectionIndex != -1) {
      sym->sectionIndex = file->sections.at(sym->sectionIndex)->sectionNumber;
    }

    Symbol *symInFirstFile = nullptr;
    for (Symbol *sym1 : symTab) {
      if (sym1->name == sym->name) {
        symInFirstFile = sym1;
        break;
      }
    }
    if (symInFirstFile == nullptr) {
      symTab.push_back(sym);
    } else {
      if (symInFirstFile->type != SymbolType::EXTERN &&
          sym->type != SymbolType::EXTERN) {
        fmt::println(stderr,
                     "[ERROR] Symbol \"{}\" from file \"{}\" is alredy defined "
                     "in file \"{}\" ",
                     sym->name, file->name, this->name);
        exit(0);
      }
      if (symInFirstFile->type == SymbolType::EXTERN) {
        symInFirstFile->type = sym->type;
        symInFirstFile->value = sym->value;
        symInFirstFile->sectionIndex = sym->sectionIndex;
      }
    }
  }

  // sectionOffsetIndex
  for (int i = backPatchesInFirstFile; i < backPatches.size(); i++) {
    BackPatch *bp = backPatches.at(i);

    if (bp->type != BackPatchType::SectionOffset) {
      continue;
    }

    bp->sectionOffsetIndex =
        file->sections.at(bp->sectionOffsetIndex)->sectionNumber;
  }

  // add remaining backPatches
  for (BackPatch *bp : file->backPatches) {
    bp->sectionIndex = file->sections.at(bp->sectionIndex)->sectionNumber;
    if (bp->type == BackPatchType::SectionOffset) {
      bp->sectionOffsetIndex =
          file->sections.at(bp->sectionOffsetIndex)->sectionNumber;
    }
    if (bp->sym != nullptr) {
      for (Symbol *sym : symTab) {
        if (sym->name == bp->sym->name) {
          bp->sym = sym;
          break;
        }
      }
    }
    backPatches.push_back(bp);
  }

  for (int i = 0; i < sections.size(); i++) {
    fmt::println(stderr, "{}", *sections.at(i));
  }
  for (int i = 0; i < backPatches.size(); i++) {
    fmt::println(stderr, "{}", *backPatches.at(i));
  }
  fmt::println(stderr, "=======AFTER====================");
}

void ObjFile::mergeSections(int firstIndex, ObjFile *file, int secondIndex) {
  Section *sec1 = sections.at(firstIndex);
  Section *sec2 = file->sections.at(secondIndex);

  // Merging backPatches
  for (int i = file->backPatches.size() - 1; i >= 0; i--) {
    BackPatch *bp = file->backPatches.at(i);
    if (bp->sectionIndex == secondIndex) {
      backPatches.push_back(bp);
      sec2->sectionMemory.at(bp->position) += sec1->sectionMemory.size();
      bp->position += sec1->sectionMemory.size();
      bp->sectionIndex = firstIndex;

      if (bp->sym != nullptr) {
        for (Symbol *firstFileSym : symTab) {
          if (firstFileSym->name == bp->sym->name) {
            bp->sym = firstFileSym;
            break;
          }
        }
      }

      file->backPatches.erase(file->backPatches.begin() + i);
    }
  }

  // Merging symbols
  for (int i = file->symTab.size() - 1; i >= 0; i--) {
    Symbol *sym = file->symTab.at(i);
    if (sym->sectionIndex == secondIndex) {

      sym->sectionIndex = firstIndex;
      if (sym->type == SymbolType::RELOC) {
        sym->value += sec1->sectionMemory.size();
      }

      Symbol *symInFirstFile = nullptr;
      for (Symbol *s : symTab) {
        fmt::println("SYMMERGE:{}|{}", s->name, sym->name);
        if (s->name == sym->name) {
          symInFirstFile = s;
          break;
        }
      }

      if (symInFirstFile != nullptr) {
        if (symInFirstFile->type != SymbolType::EXTERN &&
            sym->type != SymbolType::EXTERN) {
          fmt::println(
              stderr,
              "[ERROR] Symbol \"{}\" from file \"{}\" is alredy defined "
              "in file \"{}\" ",
              sym->name, file->name, this->name);
          exit(0);
        }
        if (symInFirstFile->type == SymbolType::EXTERN) {
          symInFirstFile->type = sym->type;
          symInFirstFile->value = sym->value;
          symInFirstFile->sectionIndex = firstIndex;
        }
      } else {
        sym->sectionIndex = firstIndex;
        symTab.push_back(sym);
      }

      file->symTab.erase(file->symTab.begin() + i);
    }
  }

  // merging sectionMemory
  for (int i = 0; i < sec2->sectionMemory.size(); i++) {
    sec1->sectionMemory.push_back(sec2->sectionMemory.at(i));
  }

  file->sections.at(secondIndex)->sectionNumber = -1;
  // file->sections.at(secondIndex)->sectionNumber = firstIndex;
}

void ObjFile::patchUnknownSymbols(bool mustPatchAll) {
  for (int i = backPatches.size() - 1; i >= 0; i--) {
    BackPatch *bp = backPatches.at(i);
    if (bp->type != BackPatchType::UnknownSymbol) {
      continue;
    }
    Symbol *sym = bp->sym;
    if (sym->type == SymbolType::EXTERN) {
      if (mustPatchAll == true) {
        fmt::println(stderr, "[ERROR] Symbol \"{}\" is not defined");
        exit(0);
      }
      continue;
    }

    Section *sec = sections.at(bp->sectionIndex);

    if (sym->type == SymbolType::ABS) {
      int *memPtr = (int *)&sec->sectionMemory.at(bp->position);
      *memPtr = sym->value;
    } else if (sym->type == SymbolType::RELOC) {
      int *memPtr = (int *)&sec->sectionMemory.at(bp->position);
      *memPtr = sym->value;

      BackPatch *newReloc = new BackPatch();
      newReloc->sectionIndex = bp->sectionIndex;
      newReloc->position = bp->position;
      newReloc->type = BackPatchType::SectionOffset;
      newReloc->sectionOffsetIndex = sym->sectionIndex;
      backPatches.push_back(newReloc);
    }
    backPatches.erase(backPatches.begin() + i);
  }
}

void ObjFile::setOffset(Section *sec, uint32_t offset) {
  if (sec->offset != 0xffffffff) {
    fmt::println(stderr, "[ERROR] redefinition of offset of section \"{}\" ",
                 sec->name);
    exit(0);
  }
  sec->offset = offset;

  /*for (int i = backPatches.size() - 1; i >= 0; i--) {
    BackPatch *bp = backPatches.at(i);
    if (sec->sectionNumber == bp->sectionIndex) {
      if (bp->type != BackPatchType::SectionOffset) {
        fmt::println(stderr, "[ERROR] backpatch type not SectionOffset");
        exit(0);
      }

      uint32_t *location = (uint32_t *)&sec->sectionMemory.at(bp->position);
      *location += offset;
      backPatches.erase(backPatches.begin() + i);
    }
  }*/
}

Section *ObjFile::findClosestSection(uint32_t address) {
  Section *closest = nullptr;

  uint32_t minDifference = 0xffffffff;

  for (Section *s : sections) {
    if (s->offset == 0xffffffff) {
      continue;
    }
    if (s->offset < address) {
      continue;
    }

    if (s->offset - address < minDifference) {
      closest = s;
      minDifference = s->offset - address;
    }
  }

  return closest;
}
void ObjFile::createExecutable(char *filepath,
                               std::vector<PlaceDirective> &placeDirectives) {
  // set offset for -place
  for (PlaceDirective &pd : placeDirectives) {
    bool found = false;
    for (Section *sec : sections) {
      if (pd.secName == sec->name) {
        setOffset(sec, pd.adress);
        found = true;
        break;
      }
    }
    if (found == false) {
      fmt::println(stderr, "[ERROR] Section \"{}\" not found", pd.secName);
      exit(0);
    }
  }

  // check if sections overlap
  for (Section *s1 : sections) {
    if (s1->offset == 0xffffffff) {
      continue;
    }
    for (Section *s2 : sections) {
      if (s1 == s2) {
        continue;
      }
      if (s2->offset == 0xffffffff) {
        continue;
      }

      if (s1->offset < s2->offset + s2->sectionMemory.size() &&
          s1->offset > s2->offset) {
        fmt::println(stderr, "[ERROR] Sections \"{}\" and \"{}\" overlap",
                     s1->name, s2->name);
        exit(0);
      }
    }
  }

  // set offset for other sections
  uint32_t counter = 0;
  for (Section *sec : sections) {
    if (sec->offset != 0xffffffff) {
      continue;
    }

    Section *closestSection = findClosestSection(counter);
    while (counter + sec->sectionMemory.size() > closestSection->offset &&
           closestSection != nullptr) {
      counter = closestSection->offset + closestSection->sectionMemory.size();
      closestSection = findClosestSection(counter);
    }

    setOffset(sec, counter);
    counter += sec->sectionMemory.size();
  }

  for (int i = backPatches.size() - 1; i >= 0; i--) {
    BackPatch *bp = backPatches.at(i);
    Section *sec = sections.at(bp->sectionIndex);

    if (bp->type != BackPatchType::SectionOffset) {
      fmt::println(stderr, "[ERROR] backpatch type not SectionOffset");
      exit(0);
    }

    uint32_t offset = sections.at(bp->sectionOffsetIndex)->offset;
    uint32_t *location = (uint32_t *)&sec->sectionMemory.at(bp->position);
    *location += offset;
    backPatches.erase(backPatches.begin() + i);
  }

  dumpToFile("TESTEST");

  FILE *f = fopen(filepath, "w");
  if (f == nullptr) {
    fmt::println(stderr, "[ERROR]Error with creating file");
    exit(0);
  }
  for (Section *sec : sections) {
    fmt::println(f, "{:#x} {:#x}", sec->offset, sec->sectionMemory.size());
    for (int i = 0; i < sec->sectionMemory.size(); i++) {
      fmt::print(f, "{:#x} ", sec->sectionMemory.at(i));
      if (i % 4 == 3) {
        fmt::print(f, "\n");
      }
    }
    fmt::println(f, "");
  }
}
