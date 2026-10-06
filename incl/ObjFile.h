#ifndef OBJ_FILE_H
#define OBJ_FILE_H

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
class Symbol;
class Section;
class BackPatch;
class fstream;

struct PlaceDirective {
  std::string secName;
  unsigned int adress;
};
/*
//entry for symbol
sym name index defined type value

//entry for backPatch
//if sym == "#" sym = nullptr
bp type sectionIndex position sym


//entry for section
sec name memorySize
byte0 byte1 byte3 byte 4
...    ...   ...   ...
byteN
*/
struct ObjFile {
  std::vector<Symbol *> symTab;
  std::vector<Section *> sections;
  std::vector<BackPatch *> backPatches;
  std::string name;

  ObjFile(char *filePath);
  ObjFile();

  void dumpToFile(char *filepath);
  void createExecutable(char *filepath,
                        std::vector<PlaceDirective> &placeDirectives);

  void mergeWithObjFile(ObjFile *file);

  void patchUnknownSymbols(bool mustPatchAll = false);
  void setOffset(Section *sec, uint32_t offset);

private:
  Section *findClosestSection(uint32_t address);
  void parseSymbol(std::fstream &input);
  void parseBackPatch(std::fstream &input);
  void parseSection(std::fstream &input);

  void mergeSections(int firstIndex, ObjFile *file, int secondIndex);
};

#endif // !OBJ_FILE_H
