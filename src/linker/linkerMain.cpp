#define FMT_HEADER_ONLY
#include "../../incl/ObjFile.h"
#include "../../incl/fmt/base.h"
#include "../../incl/fmt/core.h"
#include "../../incl/misc.h"
#include "../../incl/symbol.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <vector>

std::vector<ObjFile *> files;

ObjFile finalFile;

int main(int argc, char *argv[]) {

  // Command line arguments
  bool outputNameNext = false;
  char *outputName = nullptr;
  bool relocatable = false;
  bool hex = false;

  std::vector<PlaceDirective> placeDirectioves;

  for (int i = 1; i < argc; i++) {
    fmt::println("argv[{}]:{}", i, argv[i]);
    std::string str = argv[i];
    if (outputNameNext) {
      outputName = argv[i];
      outputNameNext = false;
      continue;
    }
    if (str == "-o") {
      if (outputName != nullptr) {
        fmt::println(stderr, "[ERROR] \"-o\" alredy set");
        exit(0);
      }
      if (i == argc - 1) {
        fmt::println(stderr, "[ERROR] output name not specified");
        exit(0);
      }
      outputNameNext = true;
      continue;
    }
    if (str == "-hex") {
      if (relocatable) {
        fmt::println(stderr,
                     "[ERROR] -hex and -relocatable cant be used at same time");
        exit(0);
      }
      hex = true;
      continue;
    }
    if (str == "-relocatable") {
      if (hex) {
        fmt::println(stderr,
                     "[ERROR] -hex and -relocatable cant be used at same time");
        exit(0);
      }
      relocatable = true;
      continue;
    }

    if (str.find("-place=") == 0) {
      str.erase(0, 7);
      fmt::println("{}", str);
      int atIndex = str.find("@");
      if (atIndex == std::string::npos) {
        fmt::println(stderr, "[ERROR] not '@' in -place");
        exit(0);
      }

      std::string secName = str;
      secName.erase(atIndex);
      std::string adressStr = str;
      adressStr.erase(0, atIndex + 1);

      fmt::println("adrStr:{}", adressStr);
      int base = isNumber(adressStr);
      if (base == 0) {
        fmt::println(stderr, "[ERROR] address is not number in -place");
        exit(0);
      }

      unsigned int adress = std::stoul(adressStr, 0, base);
      placeDirectioves.push_back({secName, adress});
      continue;
    }

    files.push_back(new ObjFile(argv[i]));
  }
  if (outputName == nullptr) {
    fmt::println(stderr, "[ERROR] No output name");
    exit(0);
  }
  if (!hex && !relocatable) {
    fmt::println(stderr, "[ERROR] both -hex and -relocatable are off");
    exit(0);
  }

  fmt::println("number of files:{}", files.size());
  for (int i = 1; i < files.size(); i++) {
    files.at(0)->mergeWithObjFile(files.at(i));
  }

  files.at(0)->dumpToFile("PRE_UNKNOWN_PATCH");

  files.at(0)->patchUnknownSymbols(hex);

  files.at(0)->dumpToFile("POST_UNKNOWN_PATCH");

  if (relocatable) {
    files.at(0)->dumpToFile(outputName);
  } else if (hex) {
    files.at(0)->createExecutable(outputName, placeDirectioves);
  }

  return 0;
}
