#include <cstdint>
#include <memory>
#include <sched.h>

#include "common.h"
#include "node.h"

RTNode::RTNode(ArenaHeader *ah, std::unique_ptr<EnvSource> env_src,
               std::unique_ptr<Scheduler> sched)
  : ah(ah), reg(ah, std::move(env_src)), sched(std::move(sched)) {}

BlockID RTNode::alloc_block(uint64_t size) {
  BlockID id = reg.blocks.new_block(size);
  if (id != INVALID_BLOCK)
    dag.new_block_hook(id);
  return id;
}

EnvID RTNode::declare_env(const std::string& ref) {
  return reg.envs.resolve_or_insert(ref);
}

void RTNode::submit(const TaskSpec& spec) {
  dag.submit_hook(spec);
}

void RTNode::dispatch_ready_tasks() {
  auto ready = dag.take_ready_tasks();
  for (auto it = ready.rbegin(); it != ready.rend(); it++) {
    TaskSpec task = dag.spec(*it);
    ExecID tgt = sched->place(task, *ah);
    task.enqueue_ts = now_ns();

    while (!ah->dispatch_ring[tgt].push(task))
      sched_yield();
  }
}

void RTNode::join_completed_tasks() {
  TaskTrace tr;
  while (ah->completion_ring.pop(tr))
    dag.completion_hook(tr);
}

void RTNode::dump_dot(std::ostream& os) const {
  dag.dump_dot(os, [this](EnvID env, KernelID k) {
    return reg.envs.get_kernel_name(env, k);
  });
}

void RTNode::dump_jsonl(std::ostream& os) const {
  dag.dump_jsonl(os, [this](EnvID env, KernelID k) {
    return reg.envs.get_kernel_name(env, k);
  });
}
