#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <cstdio>
#include <string>
#include <type_traits>

class Tokenizer {
public:
  Tokenizer(char *fileName);
  int currentLine = 1;

  std::string getNextToken(); // returns null at files end
  void getToEnd();

private:
  FILE *f = nullptr;
};

#endif // TOKENIZER_H
