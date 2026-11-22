#include <cstdint>
#include <cstring>

#include "env_abi.h"

namespace {

// fill:
//      REF[0]: (f64) val -> MUT[1]: (f64*) dst
void k_fill(void* const* v, const uint64_t* len, uint16_t _) {
  double  val = *(const double*)v[0];
  double* dst = (double*)v[1];

  for (size_t i = 0; i < len[1] / 8; i++)
    dst[i] = val;
}

// inc:
//      MUT[0]: (f64*) cell -> MUT[0]: (f64*) cell
void k_inc(void* const* v, const uint64_t* len, uint16_t _) {
  double* cell = (double*)v[0];

  for (size_t i = 0; i < len[0] / 8; i++)
    cell[i] += 1.0;
}

// copy:
//      REF[0]: (f64*) src -> MUT[1]: (f64*) dst
void k_copy(void* const* v, const uint64_t* len, uint16_t _) {
  size_t cpy_len = len[0] < len[1] ? len[0] : len[1];
  memcpy(v[1], v[0], cpy_len);
}

} // namespace

extern "C" const uint32_t env_kernel_count = 3;
extern "C" const EnvKernelMeta env_kernel_table[] = {
  { "fill", k_fill },      // 0
  { "inc",  k_inc  },      // 1
  { "copy", k_copy },      // 2
};
