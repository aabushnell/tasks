#pragma once

#include <cstdint>
#include <unordered_map>

#include "common.h"
#include "env_source.h"

class BlockTable {
public:
  explicit BlockTable(ArenaHeader* ah) : ah(ah) {}

  BlockID new_block(uint64_t size);

  BlockDesc block_desc(BlockID id) const {
    return ah->blocks[id];
  }

  uint32_t size() const {
    return ah->n_blocks;
  }

private:
  ArenaHeader* ah;
  uint64_t next_offset = 0;
};

class EnvStore {
public:
  EnvStore(ArenaHeader* ah, std::unique_ptr<EnvSource> src)
    : ah(ah), env_src(std::move(src)) {}

  EnvID resolve_or_insert(const std::string& ref);

  std::string get_kernel_name(EnvID env, KernelID k) const;

  const char* env_path(EnvID id) const {
    return ah->envs[id].path;
  }

private:
  ArenaHeader* ah;
  std::unique_ptr<EnvSource> env_src;
  std::unordered_map<std::string, EnvID> env_ids;
  std::vector<std::vector<std::string>> kernel_names;
};

struct Registry {
  BlockTable  blocks;
  EnvStore    envs;

  Registry(ArenaHeader* ah, std::unique_ptr<EnvSource> src)
    : blocks(ah), envs(ah, std::move(src)) {}
};
