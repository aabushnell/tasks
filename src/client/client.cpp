#include <cstdio>
#include <cstring>
#include <initializer_list>

#include "client/client.h"
#include "common.h"

BlockID Client::alloc_block(uint64_t size) {
  return node.alloc_block(size);
}

EnvID Client::declare_env(std::string_view ref) {
  return node.declare_env(std::string(ref));
}

void Client::submit(const TaskSpec& spec) {
  node.submit(spec);
}

void Client::submit(EnvID env, KernelID kernel,
                    std::initializer_list<Depend> deps) {
  if (deps.size() > INLINE_DEPS) {
    fprintf(stderr, "Client::submit: declared dependencies > inline cap (%zu)\n",
            deps.size());
    return;
  }

  TaskSpec spec{ };
  spec.env = env;
  spec.kernel = kernel;
  spec.n_depends = (uint16_t)deps.size();
  memcpy(spec.dependencies, deps.begin(), deps.size() * sizeof(Depend));

  node.submit(spec);
}

void* Client::block_ptr(BlockID id) const {
  ArenaHeader* ah = node.arena_header_ptr();
  return (uint8_t *)ah + ah->blocks[id].offset;
}
