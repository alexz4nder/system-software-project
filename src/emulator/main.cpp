#define FMT_HEADER_ONLY
#include "../../incl/Bus.h"
#include "../../incl/fmt/base.h"

int main(int argc, char **argv) {

  if (argc != 2) {
    fmt::println(stderr, "[ERROR] only 1 argument required");
    return 0;
  }
  Bus *bus = new Bus(argv[1]);

  return 0;
}
