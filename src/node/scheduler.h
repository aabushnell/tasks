#pragma once

#include <random>

#include "common.h"

class Scheduler {
public:
  virtual ~Scheduler() = default;

  virtual ExecID place(const TaskSpec& spec, const ArenaHeader& ah) = 0;
};

class PO2Scheduler : public Scheduler {
public:
  ExecID place(const TaskSpec&, const ArenaHeader& ah) override {
    ExecID a = rng() % ah.n_exec;
    ExecID b = rng() % ah.n_exec;
    if (ah.dispatch_ring[a].depth() <= ah.dispatch_ring[b].depth()) {
      return a;
    } else {
      return b;
    }
  }

private:
  std::mt19937 rng{ 36 };
};
