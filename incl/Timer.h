#ifndef TIMER_H
#define TIMER_H

#include <cstdint>
#include <thread>
class Bus;
class Timer {
  uint32_t tim_cfg = 0;
  inline static uint32_t ticksForInterrupt[8] = {
      1,  // 500ms
      2,  // 1000ms
      3,  // 1500ms
      4,  // 2000ms
      10, // 5000ms
      20, // 10s
      60, // 30s
      120 // 60s
  };
  std::thread *timerThread = nullptr;
  uint32_t counter = 0;
  void timerThreadMethod();
  Bus *bus;

public:
  void writeTimer(uint32_t data);
  Timer(Bus *bus);
};

#endif // !TIMER_H
