#define FMT_HEADER_ONLY
#include "../../incl/Timer.h"
#include "../../incl/Bus.h"
#include "../../incl/fmt/base.h"
#include <chrono>
#include <thread>

Timer::Timer(Bus *bus) {
  this->bus = bus;
  timerThread = new std::thread([&] {
    fmt::println("STARTING TIMER THREAD");
    this->timerThreadMethod();
  });
}

void Timer::timerThreadMethod() {
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    counter += 1;

    if (counter == ticksForInterrupt[tim_cfg]) {
      counter = 0;
      bus->interruptTimer();
      // fmt::println("TIMER INTERRUPT");
    }
  }
}

void Timer::writeTimer(uint32_t data) {
  // fmt::println("TIMER SET:{}", data);
  tim_cfg = data & 0b111;
}
