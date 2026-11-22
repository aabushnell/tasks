#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>

#include "env_abi.h"
#include "registry.h"
#include "common.h"

namespace {

std::vector<std::string> parse_kernel_names(const std::string& path) {
  std::vector<std::string> names;
  void* h = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!h)
    return names;
  auto count = (const uint32_t*)dlsym(h, ENV_SYM_COUNT);
  auto table = (const EnvKernelMeta*)dlsym(h, ENV_SYM_TABLE);
  if (count && table && *count > 0) {
    for (size_t i = 0; i < *count; i++)
      names.emplace_back(table[i].name ? table[i].name : "???");
  }
  dlclose(h);
  return names;
}

} // namespace

BlockID BlockTable::new_block(uint64_t size) {
  if (ah->n_blocks >= MAX_BLOCKS) {
    fprintf(stderr, "BlockTable: maximum blocks exceeded\n");
    return INVALID_BLOCK;
  }

  // init offset to start of data section
  if (next_offset == 0)
    next_offset = arena_data_offset();

  BlockID id = ah->n_blocks++;
  ah->blocks[id] = { next_offset, size };
  next_offset += size;

  return id;
}

EnvID EnvStore::resolve_or_insert(const std::string& ref) {
  std::string so = env_src->resolve(ref);
  if (so.empty() || so.size() >= ENV_PATH_LEN) {
    fprintf(stderr, "EnvRegistry: invalid object path: '%s'\n", ref.c_str());
    return INVALID_ENV;
  }

  if (auto it = env_ids.find(so); it != env_ids.end())
    return it->second;

  if (ah->n_envs >= MAX_ENVS) {
    fprintf(stderr, "EnvRegistry: maximum environments exceeded\n");
    return INVALID_ENV;
  }

  EnvID id = ah->n_envs++;
  memset(ah->envs[id].path, 0, ENV_PATH_LEN);
  memcpy(ah->envs[id].path, so.data(), so.size());
  env_ids.emplace(so, id);
  kernel_names.push_back(parse_kernel_names(so));

  return id;
}

std::string EnvStore::get_kernel_name(EnvID env, KernelID k) const {
  if (env < kernel_names.size()
    && k < kernel_names[env].size())
    return kernel_names[env][k];

  // else: fallback to ids
  char buf[32];
  snprintf(buf, sizeof buf, "env%u:k%u", env, k);
  return buf;
}
