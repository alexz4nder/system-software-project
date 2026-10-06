#ifndef LITTERAL_POOL_H
#define LITTERAL_POOL_H

#include <string>
#include <vector>

class Symbol;
enum class PoolType { symbol, literal };
struct PoolEntry {
  PoolType type;
  Symbol *sym;
  int value;
  int index;
};

#endif // !LITTERAL_POOL_H
