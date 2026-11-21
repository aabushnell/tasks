#pragma once
#include <cstdint>

#include "common.h"

static constexpr Depend RD(BlockID id, uint64_t len) {
  return {id, 0, len, Read};
}
static constexpr Depend WR(BlockID id, uint64_t len) {
  return {id, 0, len, Write};
}

static constexpr Depend RD_AT(BlockID id, uint64_t off, uint64_t len) {
  return {id, off, len,  Read};
}
static constexpr Depend WR_AT(BlockID id, uint64_t off, uint64_t len) {
  return {id, off, len,  Write};
}

static constexpr Depend RD_64(BlockID id) {
  return RD(id, 8);
}
static constexpr Depend WR_64(BlockID id) {
  return WR(id, 8);
}

#define ALLOC_BLOCK(name, bytes) BlockID name = sup.alloc_block(bytes)
#define ALLOC_BLOCK_VEC(name, n, bytes)             \
    std::vector<BlockID> name((n));                 \
    for (size_t _i = 0; _i < (size_t)(n); ++_i)     \
        name[_i] = sup.alloc_block(bytes)

#define BLOCK_PTR(T, id)  ((T*)((uint8_t*)AH + AH->blocks[id].offset))

#define SUBMIT(env, k, ...)  sup.submit_task(mk(env, k, { __VA_ARGS__ }))
