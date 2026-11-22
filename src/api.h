#pragma once

#include "common.h"

static constexpr Depend REF(BlockID id) {
  return {id, Ref};
}
static constexpr Depend MUT(BlockID id) {
  return {id, Mut};
}

#define ALLOC_BLOCK(c, name, bytes)                   \
  BlockID name = (c).alloc_block(bytes)

#define ALLOC_BLOCK_ARRAY(c, name, n, bytes)          \
    std::vector<BlockID> name((n));                   \
    for (size_t _i = 0; _i < (size_t)(n); ++_i)       \
        name[_i] = (c).alloc_block(bytes)

#define BLOCK_PTR(c, T, id)                           \
  ((T*)(c).block_ptr(id))

#define SUBMIT(c, env, k, ...)                        \
  (c).submit(env, k, { __VA_ARGS__ })
