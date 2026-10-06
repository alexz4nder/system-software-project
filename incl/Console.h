#ifndef CONSOLE_H
#define CONSOLE_H

#include <cstdint>
#include <thread>
class Bus;
class Console {
  uint32_t consoleIn;
  void terminalReader();
  Bus *bus;

  std::thread *consoleThread = nullptr;

public:
  void writeConsoleOut(uint32_t character);
  uint32_t readConsoleIn();

  Console(Bus *bus);
};

#endif // !CONSOLE_H
