#pragma once

#include <atomic>
#include <vector>
#include <thread>

#include "node/node.h"

struct ExecPoolConfig {
  int n_exec = 0;
};

class Supervisor {
public:
  explicit Supervisor(RTNode& node) : node(node) {}

  void start(ExecPoolConfig cfg = {});

  void join();

  void signal_shutdown() {
    stop.store(1, std::memory_order_release);
  }

private:
  void run();
  void spawn_executors(size_t n);
  void reap_executors();

  RTNode& node;
  ExecPoolConfig cfg{ };
  std::thread thread;
  std::vector<pid_t> exec_pids;
  std::atomic<bool> stop{ false };
};
