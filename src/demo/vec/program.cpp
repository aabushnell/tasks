#include "api.h"
#include "client/client.h"
#include <cstddef>

enum kernels : uint8_t { FILL, INC, COPY };

void vec_demo(Client& c, size_t n_cells = 8,
              size_t cell_size = 4096, size_t chain_len = 1) {

  EnvID env = c.declare_env("./demo/vec/vec.so");

  // allocate memory blocks:
  /*                   name;  n;       bytes;                 */
  ALLOC_BLOCK_ARRAY(c, A,     n_cells, (uint64_t)cell_size * 8);
  ALLOC_BLOCK_ARRAY(c, B,     n_cells, (uint64_t)cell_size * 8);
  ALLOC_BLOCK_ARRAY(c, seeds, n_cells, 8 /* scalar */);

  // initialize seed values:
  for (size_t i = 0; i < n_cells; i++)
    *BLOCK_PTR(c, double, seeds[i]) = (double)i;

  // fill cells in A with seeded value:
  for (size_t i = 0; i < n_cells; i++) {
    SUBMIT(c, env,
      /* kernel: */ FILL,
      /*  value: */ REF(seeds[i]),
      /*   dest: */ MUT(A[i])
    );
  }

  // increment cells in A by 1; chain_len times:
  for (size_t i = 0; i < n_cells; i++) {
    for (size_t j = 0; j < chain_len; j++) {
      SUBMIT(c, env,
        /* kernel: */ INC,
        /*  place: */ MUT(A[i])
      );
    }
  }

  // copy finished values from A to B:
  for (size_t i = 0; i < n_cells; i++) {
    SUBMIT(c, env,
      /* kernel: */ COPY,
      /*    src: */ REF(A[i]),
      /*   dest: */ MUT(B[i])
    );
  }
}
