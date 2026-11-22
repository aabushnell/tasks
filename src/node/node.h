#pragma once

#include <memory>
#include <ostream>
#include <vector>

#include "common.h"
#include "dag.h"
#include "env_source.h"
#include "registry.h"
#include "scheduler.h"

class RTNode {
public:
  explicit RTNode(ArenaHeader* ah,
                  std::unique_ptr<EnvSource> env_source =
                    std::make_unique<LocalEnvSource>(),
                  std::unique_ptr<Scheduler> sched =
                    std::make_unique<PO2Scheduler>());

  BlockID alloc_block(uint64_t size);

  EnvID declare_env(const std::string& ref);

  void submit(const TaskSpec& spec);

  void dispatch_ready_tasks();

  void join_completed_tasks();

  void dump_dot(std::ostream& os) const;
  void dump_jsonl(std::ostream& os) const;

  const std::vector<TaskTrace>& dump_traces() const {
    return dag.get_traces();
  }

  size_t alive() const {
    return dag.n_pending();
  }

  ArenaHeader* arena_header_ptr() {
    return ah;
  }

private:
  ArenaHeader *ah;
  Registry reg;
  LocalDAG dag;
  std::unique_ptr<Scheduler> sched;
};
