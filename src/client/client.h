#pragma once

#include <initializer_list>
#include <string_view>

#include "common.h"
#include "node/node.h"

class Client {
public:
  explicit Client(RTNode& node) : node(node) {}

  BlockID alloc_block(uint64_t size);

  EnvID declare_env(std::string_view ref);

  void submit(const TaskSpec& spec);
  void submit(EnvID env, KernelID kernel,
              std::initializer_list<Depend> deps);

  void *block_ptr(BlockID) const;

  bool idle() const {
    return node.alive() == 0;
  }

private:
  RTNode& node;
};
