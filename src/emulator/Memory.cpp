#define FMT_HEADER_ONLY
#include "../../incl/Memory.h"
#include "../../incl/fmt/base.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>

Memory::Memory(char *filename) {
  memory = (char *)malloc(0xffffff00);

  std::ifstream file(filename);
  if (!file.is_open()) {
    fmt::println(stderr, "[ERROR] cant open file \"{}\"", filename);
    exit(0);
  }
  file >> std::hex;
  while (!file.eof()) {
    uint32_t adr, size;
    file >> adr;
    file >> size;

    fmt::println("adr:{:#x} size:{:#x}", adr, size);

    if (file.eof()) {
      break;
    }

    for (int i = 0; i < size; i++) {
      uint32_t data;
      file >> data;
      uint8_t data8bit = (uint8_t)data;
      *(memory + adr + i) = data8bit;
    }
  }
  fmt::println("MEMORY LOADED");

  uint32_t *ptr = (uint32_t *)(memory + 0x40000000);
  for (int i = 0; i < 16; i++) {
    fmt::println("{}", *(ptr + i));
  }
}

void Memory::write(uint32_t adr, uint32_t data) {
  *((uint32_t *)(memory + adr)) = data;
}

uint32_t Memory::read(uint32_t adr) {
  uint32_t memVal = *((uint32_t *)(memory + adr));
  // fmt::println("ADR:{:#x} VAL:{:#x}", adr, memVal);
  return memVal;
}
