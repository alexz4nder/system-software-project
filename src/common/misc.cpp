#include "../../incl/misc.h"

int isNumber(std::string &str) {
  int base = 10;
  int loopStart = 0;

  if (str.length() > 2 && str.at(0) == '0' && str.at(1) == 'x') {
    base = 16;
    loopStart = 2;
  }

  for (int i = loopStart; i < str.length(); i++) {
    char ch = str.at(i);
    if (ch < '0' || ch > '9') {

      if (base == 16 &&
          ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))) {
        continue;
      }
      return 0;
    }
  }

  return base;
}
