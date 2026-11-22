#pragma once

#include <cstdint>
#include <functional>
#include <ostream>
#include <vector>

#include "common.h"

using NameResolver = std::function<std::string(EnvID, KernelID)>;

class LocalDAG {
public:
  struct BlockCursor {
    TaskID producer = 0; /* NOTE: 0 = None */
    std::vector<TaskID> consumers;
  };

  struct TaskVertex {
    TaskSpec spec;
    std::vector<TaskID> predecessors;
    std::vector<TaskID> successors;
    uint32_t n_pending = 0;
  };

  void new_block_hook(BlockID);
  void submit_hook(const TaskSpec&);
  std::vector<TaskID> completion_hook(const TaskTrace&);

  std::vector<TaskID> take_ready_tasks();

  void dump_dot(std::ostream&, const NameResolver& = {}) const;
  void dump_jsonl(std::ostream&, const NameResolver& = {}) const;

  const TaskSpec& spec(TaskID id) const {
    return vertices[id - 1].spec;
  }

  size_t n_submitted() const {
    return vertices.size();
  }

  size_t n_pending() const {
    return vertices.size() - n_completed;
  }

  const std::vector<TaskTrace>& get_traces() const {
    return traces;
  }

private:
  uint32_t infer_edges(struct TaskVertex& node);

  std::vector<BlockCursor>  frontier;
  std::vector<TaskVertex>   vertices;

  std::vector<TaskID>       ready_tasks;
  std::vector<TaskTrace>    traces;

  size_t n_completed = 0;
  TaskID next_task_id = 1;
};
