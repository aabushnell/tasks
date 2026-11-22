#include <algorithm>
#include <dlfcn.h>
#include <sched.h>
#include <unistd.h>

#include "executor.h"
#include "common.h"
#include "env_abi.h"

namespace {

enum EnvCacheStatus : uint8_t { Cold, Warm, Error };

struct EnvCacheEntry {
  void* handle = nullptr;
  const EnvKernelMeta* meta_table = nullptr;

  uint32_t sym_count = 0;
  EnvCacheStatus status = Cold;
};

EnvCacheEntry env_cache[MAX_ENVS];

const EnvKernelMeta* resolve_kernel(
    ArenaHeader* ah, EnvID env_id, KernelID ker_id, uint32_t* err) {

  if (env_id >= MAX_ENVS) { *err = 1; return nullptr; }

  EnvCacheEntry& entry = env_cache[env_id];
  if (entry.status == Error) { *err = 2; return nullptr; }

  if (entry.status != Warm) {
    const char* path = ah->envs[env_id].path;

    entry.handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (entry.handle) {
      auto sym_count =
        (const uint32_t*)dlsym(entry.handle, ENV_SYM_COUNT);
      auto sym_table =
        (const EnvKernelMeta*)dlsym(entry.handle, ENV_SYM_TABLE);

      if (sym_count && sym_table && *sym_count > 0) {
        entry.sym_count = *sym_count;
        entry.meta_table = sym_table;
        entry.status = Warm;
      } else {
        dlclose(entry.handle);
        entry.handle = nullptr;
      }
    }
    if (!entry.handle) {
      fprintf(stderr, "exec: env %u (%s) failed to load: %s\n",
              env_id, path, dlerror());
      entry.status = Error;
      *err = 2;
      return nullptr;
    }
  }

  if (ker_id >= entry.sym_count) { *err = 3; return nullptr; }

  return &entry.meta_table[ker_id];
}

} // namespace

[[noreturn]] void executor_main(ArenaHeader *ah, int idx) {
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(idx % std::max(1L, sysconf(_SC_NPROCESSORS_ONLN)), &set);
  sched_setaffinity(0, sizeof(set), &set); // best effort

  auto &ring = ah->dispatch_ring[idx];
  while (true) {
    // check for shutdown order
    if (ah->shutdown.load(std::memory_order_acquire))
      _exit(0);

    // try to pop a task
    TaskSpec task;
    if (!ring.pop(task)) {
      sched_yield();
      continue;
    }
    TaskTrace rec{
      .id = task.id,
      .enqueue_ts = task.enqueue_ts,
    };

    // mark task pop
    rec.pop_ts = now_ns();

    uint32_t err = 0;
    const EnvKernelMeta* kernel_desc =
      resolve_kernel(ah, task.env, task.kernel, &err);
    if (kernel_desc) {
      void *views[INLINE_DEPS];
      uint64_t lengths[INLINE_DEPS];

      for (int i = 0; i < task.n_depends; i++) {
        const BlockDesc& blk = ah->blocks[task.dependencies[i].id];
        views[i] = (uint8_t*)ah + blk.offset;
        lengths[i] = blk.size;
      }

      // mark task start
      rec.start_ts = now_ns();

      kernel_desc->fn(views, lengths, task.n_depends);

      // mark task end
      rec.end_ts = now_ns();

      rec.status = 0;
    } else {
      rec.start_ts = rec.end_ts = rec.pop_ts;
      rec.status = err;
    }

    ah->completion_ring.push(rec);
  }
}
