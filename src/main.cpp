#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <new>
#include <string>
#include <sys/mman.h>
#include <vector>

#include "common.h"
#include "client/client.h"
#include "runtime/supervisor.h"

// ---- demo prototypes ----

void vec_demo (Client&, size_t n_cells, size_t cell_size,
                size_t chain_len);
// void red_demo (Client&, int nb, int chunk);
// void chol_demo (Client&, int nt, int tb);
// void dep_demo (Client&);
// void stencil_demo (Client&);

using Args = std::vector<uint64_t>&;
struct Demo {
  const char* name;
  const char* arg_names[4];
  size_t n_args;
  void (*run)(Client&, Args);
  bool nix_env;
};

static const Demo demos[] = {
  { "vec", { "n_cells", "cell_size", "chain_len" }, 3,
    [](Client& c, Args a) { vec_demo(c, a[0], a[1], a[2]); }, false },
  // { "red",  { "n_blocks", "chunk" }, 2,
  //   [](Client& c, Args a) { red_demo(c, a[0], a[1]); }, false },
  // { "chol", { "n_tiles", "tile_size" }, 2,
  //   [](Client& c, Args a) { chol_demo(c, a[0], a[1]); }, true },
  // { "dep",  {}, 0,
  //   [](Client& c, Args)   { dep_demo(c); }, false },
};

static ArenaHeader* init_arena() {
  ArenaHeader* ah = (ArenaHeader*)mmap(
      nullptr, ARENA_SIZE, PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (ah == MAP_FAILED) { perror("mmap"); exit(EXIT_FAILURE); }
  return new (ah) ArenaHeader();
}

static void usage(const char* prog) {
  fprintf(stderr, "usage: %s <demo> [local|nix]\n", prog);
  fprintf(stderr, "demos:\n");
  for (const Demo& d : demos) {
    fprintf(stderr, "  %-8s", d.name);
    for (size_t i = 0; i < d.n_args; i++)
      fprintf(stderr, " <%s>", d.arg_names[i]);
    fprintf(stderr, "%s\n", d.nix_env ? "   (nix env source)" : "");
  }
}

static bool parse_args(char** argv, const Demo& d, Args out) {
  for (size_t i = 0; i < d.n_args; i++) {
    char* end = nullptr;
    uint64_t v = strtoull(argv[i], &end, 10);
    if (!end || *end != '\0')
      return false;
    out.push_back(v);
  }
  return true;
}

int main(int argc, char** argv) {

  // ---- arg parsing ----

  if (argc < 2) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  const std::string name = argv[1];
  const Demo* demo = nullptr;
  for (const Demo& d : demos)
    if (name == d.name) {
      demo = &d;
      break;
    }
  if (!demo) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  if ((size_t)(argc - 2) < demo->n_args) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  std::vector<uint64_t> args_vec;
  if (!parse_args(argv + 2, *demo, args_vec)) {
    fprintf(stderr, "%s: bad numeric args\n", name.c_str());
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  const std::string env_type =
      argc > (int)(2 + demo->n_args) ? argv[2 + demo->n_args]
        : (demo->nix_env ? "nix" : "local");

  // ---- node setup ----

  ArenaHeader* ah = init_arena();
  RTNode node(ah, mk_env_source(env_type),
            std::make_unique<PO2Scheduler>());

  // ---- client setup ----

  Client client(node);
  demo->run(client, args_vec);

  // ---- runtime ----

  Supervisor sup(node);
  uint64_t t0 = now_ns();

  sup.start();
  sup.join();

  uint64_t duration = now_ns() - t0;

  // ---- stats ----

  std::vector<uint64_t> latency, execution;
  size_t errors = 0;

  for (auto& trace : node.dump_traces()) {
    latency.push_back(trace.pop_ts - trace.enqueue_ts);
    execution.push_back(trace.end_ts - trace.start_ts);
    if (trace.status != 0)
      errors++;
  }

  std::sort(latency.begin(), latency.end());
  std::sort(execution.begin(), execution.end());
  auto pct = [](std::vector<uint64_t>& v, double p) {
    return v[(size_t)(p * (v.size() - 1))];
  };

  printf("demo                 : %s\n", name.c_str());
  printf("env source           : %s\n", env_type.c_str());
  printf("tasks completed      : %zu\n", node.dump_traces().size());
  printf("errors               : %zu\n", errors);
  printf("makespan             : %.3f ms\n", duration / 1e6);
  if (!latency.empty()) {
    printf("dispatch p50 / p99   : %.1f / %.1f µs\n",
           pct(latency, .5) / 1e3,
           pct(latency, .99) / 1e3);
    printf("kernel run p50 / p99 : %.1f / %.1f µs\n",
           pct(execution, .5) / 1e3,
           pct(execution, .99) / 1e3);
  }

  // ---- trace dumps ----

  std::ofstream dot(name + ".dot");
  node.dump_dot(dot);

  std::ofstream jl(name + ".jsonl");
  node.dump_jsonl(jl);

  return errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
