#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <cstdlib>
class Memory {

  char *memory;

public:
  Memory(char *filename);

  void write(uint32_t adr, uint32_t data);
  uint32_t read(uint32_t adr);
};

#endif // !MEMORY_H
