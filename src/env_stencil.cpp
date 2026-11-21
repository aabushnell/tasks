#include "env_abi.h"

// borrows:
//      [0] = 8-byte seed (read)
//      [1] = out (write),
static void k_init(void* const* v, const uint64_t* l, uint16_t _) {
    uint32_t seed = (uint32_t)*(const int64_t*)v[0];
    int64_t* Out = (int64_t*)v[1];

    size_t n = l[0] / 8;
    for (size_t i = 0; i < n; i++)
        Out[i] = (int64_t)((seed * 2654435761u + i) % 1000);
}

// borrows:
//      [0..2] = left/mid/right (read),
//      [3] = out (write)
static void k_step(void* const* v, const uint64_t* l, uint16_t _) {
    const int64_t *Left = (const int64_t*)v[0],
                  *Middle = (const int64_t*)v[1],
                  *Right = (const int64_t*)v[2];
    int64_t* Out = (int64_t*)v[3];

    size_t n = l[3] / 8;
    for (size_t i = 0; i < n; i++)
        Out[i] = (Left[i] + 2 * Middle[i] + Right[i]) / 4;
}

// borrows:
//      [0..n-2] = partials (read),
//      [n-1] = scalar (write)
static void k_sum(void* const* v, const uint64_t* l, uint16_t nv) {
    int64_t sum = 0;
    for (uint16_t block = 0; block + 1 < nv; block++) {
        const int64_t* In = (const int64_t*)v[block];
        for (size_t i = 0; i < l[block] / 8; i++)
            sum += In[i];
    }

    *(int64_t*)v[nv - 1] = sum;
}

extern "C" const uint32_t env_kernel_count = 3;
extern "C" const EnvKernelMeta env_kernel_table[] = {
    { "init",         k_init },
    { "stencil_step", k_step },
    { "reduce_sum",   k_sum  },
};
