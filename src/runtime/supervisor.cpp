#include <algorithm>
#include <sys/wait.h>
#include <unistd.h>

#include "executor.h"
#include "supervisor.h"
#include "common.h"

void Supervisor::start(ExecPoolConfig cfg) {
  this->cfg = cfg;
  spawn_executors(cfg.n_exec);

  thread = std::thread([this] {
    run();
  });
}

void Supervisor::spawn_executors(size_t n) {
  ArenaHeader* ah = node.arena_header_ptr();

  if (n <= 0) {
    long hw = sysconf(_SC_NPROCESSORS_ONLN);
    n = (int)std::min<long>(std::max(1L, hw), MAX_EXEC);
  }
  ah->n_exec = n;

  for (size_t e = 0; e < n; e++) {
    pid_t pid = fork();
    if (pid == 0)
      executor_main(ah, e);   // [[noreturn]]
    exec_pids.push_back(pid);
  }
}

void Supervisor::run() {
  while (!stop.load(std::memory_order_acquire) && node.alive() > 0) {
    node.dispatch_ready_tasks();
    node.join_completed_tasks();
    sched_yield();
  }
}

void Supervisor::join() {
  if (thread.joinable())
    thread.join();
  reap_executors();
}

void Supervisor::reap_executors() {
  node.arena_header_ptr()->shutdown.store(1, std::memory_order_release);
  for (pid_t pid : exec_pids)
    waitpid(pid, nullptr, 0);
  exec_pids.clear();
}
