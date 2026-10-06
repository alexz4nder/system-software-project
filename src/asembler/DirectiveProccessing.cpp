#define FMT_HEADER_ONLY
#include "../../incl/DirectiveProccessing.h"
#include "../../incl/fmt/base.h"
#include "../../incl/misc.h"
#include "../../incl/sections.h"
#include "../../incl/symbol.h"
#include "../../incl/tokenizer.h"
#include "../../incl/tokens.h"
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

extern Section *current;
extern Symbol *lastSymbol;
extern std::vector<Section *> sections;

DirectiveName getDirectiveName(std::string &str);

void proccessSection(Tokenizer &tokenizer);
void proccessGlobal(Tokenizer &tokenizer);
void proccessExternal(Tokenizer &tokenizer);
void proccessEqu(Tokenizer &tokenizer);
void proccessWord(Tokenizer &tokenizer);
void proccessAscii(Tokenizer &tokenizer);
void proccessSkip(Tokenizer &tokenizer);

// returns 10 on base 10 number, 16 on base 16
// number and 0 otherwise

void proccessDirective(std::string &token, Tokenizer &tokenizer) {
  DirectiveName name = getDirectiveName(token);
  switch (name) {
  case DirectiveName::error:
    fmt::println(stderr, "[ERROR line:{}]: DIRECTIVE \"{}\" DOES NOT EXIST",
                 tokenizer.currentLine, token);
    exit(0);

  case DirectiveName::section:
    proccessSection(tokenizer);
    return;
  case DirectiveName::global:
    proccessGlobal(tokenizer);
    return;
  case DirectiveName::external:
    proccessExternal(tokenizer);
    return;
  case DirectiveName::equ:
    proccessEqu(tokenizer);
    return;
  case DirectiveName::end:
    tokenizer.getToEnd();
    if (current != nullptr) {
      current->insertPoolTable();
    }
    return;
  case DirectiveName::word:
    proccessWord(tokenizer);
    return;
  case DirectiveName::ascii:
    proccessAscii(tokenizer);
    return;
  case DirectiveName::skip:
    proccessSkip(tokenizer);
    return;
  }
}

void proccessSection(Tokenizer &tokenizer) {
  std::string sectionName = tokenizer.getNextToken();
  if (sectionName == "" || sectionName == "\n") {
    fmt::println(stderr, "[ERROR line:{}]: NO SECTION NAME",
                 tokenizer.currentLine);
    exit(0);
  }

  if (current != nullptr) {
    current->insertPoolTable();
  }

  current = new Section();
  current->name = sectionName;
  for (Section *sec : sections) {
    if (current->name == sec->name) {
      fmt::println(stderr, "[ERROR line:{}] section named \"{}\" alredy exists",
                   tokenizer.currentLine, current->name);
      exit(0);
    }
  }

  current->sectionNumber = sections.size();
  sections.push_back(current);

  std::string endLine = tokenizer.getNextToken();
  if (endLine != "\n") {
    fmt::println(stderr,
                 "[ERROR line:{}] .section directive requires only 1 argument",
                 tokenizer.currentLine);
    exit(0);
  }
}

void proccessGlobal(Tokenizer &tokenizer) {
  std::string symName = tokenizer.getNextToken();
  if (symName == "," || symName == "\n") {
    fmt::println(stderr, "[ERROR line:{}]: no symbol names in .global",
                 tokenizer.currentLine);
    exit(0);
  }

  while (true) {

    bool found = false;
    for (Symbol *sym : symTable) {
      if (symName == sym->name) {
        sym->scope = SymbolScope::GLOB;
        found = true;
      }
    }
    if (!found) {
      Symbol *sym = new Symbol();
      sym->scope = SymbolScope::GLOB;
      sym->type = SymbolType::UKN;
      sym->defined = SymbolDefined::Undefined;
      sym->name = symName;
      sym->value = 0;
      sym->sectionIndex = -1;
      symTable.push_back(sym);
    }

    std::string nextToken = tokenizer.getNextToken();
    if (nextToken == "\n") {
      return;
    }
    if (nextToken != ",") {
      fmt::println(stderr,
                   "[ERROR line:{}]: .global args not separated with coma",
                   tokenizer.currentLine);
      exit(0);
    }

    symName = tokenizer.getNextToken();
  }
}

void proccessExternal(Tokenizer &tokenizer) {
  std::string symName = tokenizer.getNextToken();
  if (symName == "," || symName == "\n") {
    fmt::println(stderr, "[ERROR line:{}] no symbol names in .extern",
                 tokenizer.currentLine);
    exit(0);
  }

  while (true) {

    bool found = false;
    for (Symbol *sym : symTable) {
      if (symName == sym->name) {
        fmt::println(stderr,
                     "[ERROR line:{}] .extern symbol alredy exists in file",
                     tokenizer.currentLine);
      }
    }
    if (!found) {
      Symbol *sym = new Symbol();
      sym->scope = SymbolScope::GLOB;
      sym->type = SymbolType::EXTERN;
      sym->name = symName;
      sym->defined = SymbolDefined::Defined;
      sym->value = 0;
      sym->sectionIndex = -1;
      symTable.push_back(sym);
    }

    std::string nextToken = tokenizer.getNextToken();
    if (nextToken == "\n") {
      return;
    }
    if (nextToken != ",") {
      fmt::println(stderr,
                   "[ERROR line:{}]: .extern args not separated with coma",
                   tokenizer.currentLine);
      exit(0);
    }

    symName = tokenizer.getNextToken();
  }
}

void proccessWord(Tokenizer &tokenizer) {
  if (current == nullptr) {
    fmt::println(stderr, "[ERROR line:{}] used .word outside setction",
                 tokenizer.currentLine);
    exit(0);
  }

  std::string word = tokenizer.getNextToken();
  if (word == "," || word == "\n") {
    fmt::println(stderr, "[ERROR line:{}] no symbol name or literal in .word",
                 tokenizer.currentLine);
    exit(0);
  }

  if (current->codeStarted != -1) {
    if (lastSymbol != nullptr &&
        lastSymbol->sectionIndex == current->sectionNumber &&
        lastSymbol->type == SymbolType::RELOC &&
        lastSymbol->value == current->sectionMemory.size()) {

      lastSymbol->value =
          current->sectionMemory.size() + current->poolTable.size() * 4;
      current->insertPoolTable();
    } else {
      current->insertPoolTable();
    }
  }

  while (true) {
    int wordType = isNumber(word);

    fmt::println("proccessiing word type {}", wordType);
    if (wordType == 0) { // symbol
      // TODO Need to create relocation first
    } else { // number litteral
      fmt::println("stoi {}", word);
      unsigned int number = std::stoul(word, 0, wordType);
      for (size_t i = 0; i < 4; i++) {
        current->sectionMemory.push_back(
            (unsigned char)(number >> (i * 8) & 0xff));
      }
    }

    std::string nextToken = tokenizer.getNextToken();
    if (nextToken == "\n") {
      return;
    }
    if (nextToken != ",") {
      fmt::println(stderr, "[ERROR line:{}] .word args not separated with coma",
                   tokenizer.currentLine);
      exit(0);
    }

    word = tokenizer.getNextToken();
  }
}
void proccessSkip(Tokenizer &tokenizer) {
  std::string size = tokenizer.getNextToken();
  if (current == nullptr) {
    fmt::println(stderr, "[ERROR line:{}] .skip called outside section",
                 tokenizer.currentLine);
    exit(0);
  }
  int sizeToSkip = 0;
  try {

    int base = 10;
    if (size.size() >= 2 && size.at(1) == 'x') {
      base = 16;
    }
    sizeToSkip = std::stoi(size, 0, base);

  } catch (std::invalid_argument) {
    fmt::println(stderr, "[ERROR line:{}] .skip size is not a number",
                 tokenizer.currentLine);
    exit(0);
  }

  if (current->codeStarted != -1) {
    if (lastSymbol != nullptr &&
        lastSymbol->sectionIndex == current->sectionNumber &&
        lastSymbol->type == SymbolType::RELOC &&
        lastSymbol->value == current->sectionMemory.size()) {

      lastSymbol->value =
          current->sectionMemory.size() + current->poolTable.size() * 4;
      current->insertPoolTable();
    } else {
      current->insertPoolTable();
    }
  }

  for (int i = 0; i < sizeToSkip; i++) {
    current->sectionMemory.push_back(0);
  }
}
void proccessAscii(Tokenizer &tokenizer) {
  std::string asciiString = tokenizer.getNextToken();

  if (asciiString.at(0) != '"' ||
      asciiString.at(asciiString.size() - 1) != '"') {
    fmt::println(stderr, "[ERROR line:{}] expected string literal",
                 tokenizer.currentLine);
    exit(0);
  }

  asciiString.erase(0, 1);
  asciiString.erase(asciiString.size() - 1, 1);

  std::string newLine = tokenizer.getNextToken();
  if (newLine != "\n") {
    fmt::println(stderr, "[ERROR line:{}] expected new line ",
                 tokenizer.currentLine);
    exit(0);
  }

  if (current->codeStarted != -1) {
    if (lastSymbol != nullptr &&
        lastSymbol->sectionIndex == current->sectionNumber &&
        lastSymbol->type == SymbolType::RELOC &&
        lastSymbol->value == current->sectionMemory.size()) {

      lastSymbol->value =
          current->sectionMemory.size() + current->poolTable.size() * 4;
      current->insertPoolTable();
    } else {
      current->insertPoolTable();
    }
  }

  for (int i = 0; i < asciiString.size(); i++) {
    current->sectionMemory.push_back(asciiString.at(i));
  }
  // TODO pitati asistenta da li string treba da bude null terminated
}

int getTokenStackPriority(std::string &token) {
  if (token == "+" || token == "-") {
    return 2;
  }
  if (token == "*" || token == "/") {
    return 3;
  }
  if (token == "(") {
    return 1;
  }
  if (token == ")") {
    return 0;
  }

  return 0;
}
int getTokenInPriority(std::string &token) {
  if (token == "+" || token == "-") {
    return 2;
  }
  if (token == "*" || token == "/") {
    return 3;
  }
  if (token == "(") {
    return 4;
  }
  if (token == ")") {
    return 1;
  }

  return 0;
}
void proccessEqu(Tokenizer &tokenizer) {
  std::string symName = tokenizer.getNextToken();
  if (symName == "\n") {
    fmt::println(stderr, "[ERROR line:{}] expected symbol name",
                 tokenizer.currentLine);
    exit(0);
  }

  std::string coma = tokenizer.getNextToken();
  if (coma != ",") {
    fmt::println(stderr, "[ERROR line:{}] expected coma",
                 tokenizer.currentLine);
    exit(0);
  }

  std::string postfixExpr = "";

  std::string nextToken;
  std::string lastToken = "";
  std::stack<std::string> stack;

  enum class ExpectedToken { OPERAND, VALUE } expected = ExpectedToken::VALUE;

  do {
    nextToken = tokenizer.getNextToken();

    if (nextToken == "(") {
      if (expected == ExpectedToken::OPERAND) {
        fmt::println(stderr, "[ERROR line:{}] cant parse expression",
                     tokenizer.currentLine);
        exit(0);
      }
      stack.push(nextToken);
    } else if (nextToken == ")") {
      while (!stack.empty()) {
        if (stack.top() == "(") {
          stack.pop();
          break;
        }
        postfixExpr += " " + stack.top();
        stack.pop();
      }
    } else if (nextToken == "+" || nextToken == "-" || nextToken == "*" ||
               nextToken == "/") {
      if (expected != ExpectedToken::OPERAND) {
        fmt::println(stderr, "[ERROR line:{}] error parsing expr",
                     tokenizer.currentLine);
        fmt::println("ExPR:{}", postfixExpr);
        exit(0);
      }
      expected = ExpectedToken::VALUE;
      int priority = getTokenInPriority(nextToken);
      if (stack.empty()) {
        stack.push(nextToken);
      } else {
        if (priority <= getTokenStackPriority(stack.top())) {
          postfixExpr += " " + stack.top();
          stack.pop();
        }
        stack.push(nextToken);
      }
    } else if (nextToken == "\n") {
      if (expected == ExpectedToken::VALUE) {

        fmt::println(stderr, "[ERROR line:{}] expression error",
                     tokenizer.currentLine);
        exit(0);
      }
      if (!stack.empty()) {
        postfixExpr += " " + stack.top();
        stack.pop();
        if (!stack.empty()) {
          fmt::println(stderr, "[ERROR line:{}] error parsing expression",
                       tokenizer.currentLine);
          exit(0);
        }
      }

    } else {
      postfixExpr += " " + nextToken;
      expected = ExpectedToken::OPERAND;
    }

  } while (nextToken != "\n");

  Symbol *sym = findSymbol(symName);
  bool symFound = sym == nullptr ? false : true;
  if (sym == nullptr) {
    sym = new Symbol();
    sym->defined = SymbolDefined::Undefined;
  }
  if (sym->defined != SymbolDefined::Undefined) {
    fmt::println(stderr, "[ERROR line:{}] symbol alredy defined",
                 tokenizer.currentLine);
    exit(0);
  }

  sym->defined = SymbolDefined::Defined;
  sym->type = SymbolType::EXPR;
  sym->name = symName;
  sym->sectionIndex = -1;
  sym->expr = postfixExpr;
  if (symFound == false) {
    symTable.push_back(sym);
  }
}

DirectiveName getDirectiveName(std::string &str) {
  if (str == ".global") {
    return DirectiveName::global;
  }
  if (str == ".section") {
    return DirectiveName::section;
  }
  if (str == ".equ") {
    return DirectiveName::equ;
  }
  if (str == ".skip") {
    return DirectiveName::skip;
  }
  if (str == ".extern") {
    return DirectiveName::external;
  }
  if (str == ".ascii") {
    return DirectiveName::ascii;
  }
  if (str == ".word") {
    return DirectiveName::word;
  }
  if (str == ".end") {
    return DirectiveName::end;
  }

  return DirectiveName::error;
}
