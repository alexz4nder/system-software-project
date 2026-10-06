#define FMT_HEADER_ONLY
#include "../../incl/tokenizer.h"
#include "../../incl/fmt/base.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
Tokenizer::Tokenizer(char *fileName) {

  f = fopen(fileName, "r");
  if (f == nullptr) {
    fmt::println("ERROR:INPUT FILE DOESNT EXIST");
    exit(0);
  }
}

enum class TokState { empty, word, stringLiteral, comment };

std::string Tokenizer::getNextToken() {

  std::string str = "";

  TokState state = TokState::empty;
  bool stringLiteralEscape = false;

  while (1) {

    char ch = std::fgetc(f);

    switch (state) {
    case TokState::empty:
      if (ch == EOF) {
        return str;
      }

      if (ch == '\n') {
        str += '\n';
        currentLine++;
        return str;
      } else if (isblank(ch)) {

        continue;
      } else if (ch == '"') {
        state = TokState::stringLiteral;
        str += '"';
        continue;
      } else if (ch == '#') {
        state = TokState::comment;
        break;
      } else if (ch == '[') {
        str += '[';
        return str;
      } else if (ch == ']') {
        str += ']';
        return str;
      } else if (ch == ',') {
        str += ',';
        return str;
      } else if (ch == '+') {
        str += '+';
        return str;
      } else if (ch == '-') {
        str += '-';
        return str;
      } else if (ch == '*') {
        str += '*';
        return str;
      } else if (ch == '/') {
        str += '/';
        return str;
      } else if (ch == '(') {
        str += '(';
        return str;
      } else if (ch == ')') {
        str += ')';
        return str;
      }

      else {
        state = TokState::word;
        str += ch;
      }

      break;

    case TokState::stringLiteral:

      if (ch == '\n' || ch == EOF) {
        fmt::println("[ERROR line:{}]: STRING LITERAL WITHOUT END",
                     currentLine);
        exit(0);
      }
      if (stringLiteralEscape) {
        stringLiteralEscape = false;
        if (ch == 'n') {
          str += '\n';
          break;
        }
        if (ch == 't') {
          str += '\t';
          break;
        }
      } else if (ch == '"') {
        str += '"';

        return str;
      } else if (ch == '\\') {
        stringLiteralEscape = true;
        break;
      }

      str += ch;

      break;

    case TokState::word:
      if (ch == '\n') {
        fseek(f, -1, SEEK_CUR);
        return str;
      }
      if (ch == ' ') {
        return str;
      }
      if (ch == ']') {
        fseek(f, -1, SEEK_CUR);
        return str;
      }
      if (ch == '[') {
        fseek(f, -1, SEEK_CUR);
        return str;
      }
      if (ch == '(') {
        fseek(f, -1, SEEK_CUR);
        return str;
      }
      if (ch == ')') {
        fseek(f, -1, SEEK_CUR);
        return str;
      }
      if (ch == '+') {
        fseek(f, -1, SEEK_CUR);
        return str;
      }
      if (ch == '-') {
        fseek(f, -1, SEEK_CUR);
        return str;
      }
      if (ch == '*') {
        fseek(f, -1, SEEK_CUR);
        return str;
      }
      if (ch == '/') {
        fseek(f, -1, SEEK_CUR);
        return str;
      }
      if (ch == '#') {
        fseek(f, -1, SEEK_CUR);
        return str;
      }
      if (ch == ',') {
        fseek(f, -1, SEEK_CUR);
        return str;
      }

      str += ch;
      break;
    case TokState::comment:
      if (ch == EOF) {
        return str;
      }
      if (ch == '\n') {
        currentLine += 1;
        return "\n";
      }
      break;
    }
  }
}

void Tokenizer::getToEnd() { fseek(f, 0, SEEK_END); }
