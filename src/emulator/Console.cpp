#include <cstdint>
#define FMT_HEADER_ONLY
#include "../../incl/Bus.h"
#include "../../incl/Console.h"
#include "../../incl/fmt/base.h"
#include <cstdio>
#include <termios.h>
#include <thread>
#include <unistd.h>

Console::Console(Bus *bus) {
  this->bus = bus;

  struct termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag &= ~ICANON;
  term.c_lflag &= ~ECHO;

  tcsetattr(STDIN_FILENO, TCSANOW, &term);

  setvbuf(stdout,NULL,_IONBF,0);

  consoleThread = new std::thread([&] {
    fmt::println("STARTING CONSOLE THREAD");
    this->terminalReader();
  });
}

void Console::writeConsoleOut(uint32_t character) {
  char ch = (char)(character & 0xff);
  /*fmt::print("CHARACTER:{:#x}", (uint32_t)ch);
  if (ch == '\r') {
    fmt::println("\nCONSOLE OUTPUT:CARRIGE RETURN");
  } else if (ch == '\n') {
    fmt::println("\nCONSOLE OUTPUT:\\n");
  } else {
    fmt::println("\nCONSOLE OUTPUT:{}", ch);
  }*/
  fmt::print("{}", ch);
}

uint32_t Console::readConsoleIn() {
  // fmt::println("READING CONSOLE_IN");
  return consoleIn;
}

void Console::terminalReader() {
  while (true) {
    char ch;
    ch = getchar();

    consoleIn = ch;
    bus->interruptConsole();
    // fmt::println("CONSOLE INPUT:{}", ch);
  }
}
