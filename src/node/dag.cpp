#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <vector>

#include "dag.h"

namespace {

std::unordered_map<TaskID, const TaskTrace*> by_id(
    const std::vector<TaskTrace>& traces) {
  std::unordered_map<TaskID, const TaskTrace*> m;
  m.reserve(traces.size());
  for (auto& tr : traces)
    m.emplace(tr.id, &tr);
  return m;
}

} // namespace

void LocalDAG::new_block_hook(BlockID) {
  frontier.push_back({ });
}

void LocalDAG::submit_hook(const TaskSpec& in) {
  TaskVertex task;
  task.spec = in;
  task.spec.id = next_task_id++;

  task.n_pending = infer_edges(task);

  vertices.push_back(std::move(task));
  if (vertices.back().n_pending == 0)
    ready_tasks.push_back(vertices.back().spec.id);
}

std::vector<TaskID> LocalDAG::completion_hook(const TaskTrace& tr) {
  std::vector<TaskID> newly_ready;
  traces.push_back(tr);
  n_completed++;

  for (TaskID s_id : vertices[tr.id - 1].successors) {
    TaskVertex& succ = vertices[s_id - 1];
    if (--succ.n_pending == 0) {
      ready_tasks.push_back(s_id);
      newly_ready.push_back(s_id);
    }
  }
  return newly_ready;
}

std::vector<TaskID> LocalDAG::take_ready_tasks() {
  std::vector<TaskID> out;
  out.swap(ready_tasks);
  return out;
}

void LocalDAG::dump_dot(std::ostream& os, const NameResolver& get_name) const {
  const auto done = by_id(traces);
    auto task_name = [&](const TaskVertex& t) {
      return get_name ? get_name(t.spec.env, t.spec.kernel)
                  : "env" + std::to_string(t.spec.env) +
                    " k"  + std::to_string(t.spec.kernel);
    };

  os << "digraph rt_dag {\n  rankdir=LR;\n  node [fontname=monospace];\n";
  for (const TaskVertex& t : vertices) {
    char deps[8 * INLINE_DEPS] = { 0 };
    char* p = deps;
    for (int i = 0; i < t.spec.n_depends; i++)
      p += snprintf(p, 8, "b%u%c ", t.spec.dependencies[i].id,
                    t.spec.dependencies[i].mode == Mut ? 'm' : 'r');

    os << "  t" << t.spec.id << " [label=\"task " << t.spec.id
       << "\\n" << task_name(t) << "\\n"
       << deps << "\"";
    if (done.count(t.spec.id))
      os << ", style=filled, fillcolor=gray85";
    os << "];\n";
    for (TaskID s : t.successors)
      os << "  t" << t.spec.id << " -> t" << s << ";\n";
  }
  os << "}\n";
}

void LocalDAG::dump_jsonl(std::ostream& os, const NameResolver& get_name) const {
  const auto done = by_id(traces);

  auto join = [](std::ostream& o, const std::vector<TaskID>& v) {
    for (size_t i = 0; i < v.size(); i++)
      o << (i ? "," : "") << v[i];
  };

  for (const TaskVertex& t : vertices) {
    os << "{\"type\":\"task\",\"id\":" << t.spec.id;
    os << ",\"env\":"    << t.spec.env;
    os << ",\"kernel\":" << t.spec.kernel;

    if (get_name) {
      os << ",\"kernel_name\":\""
         << get_name(t.spec.env, t.spec.kernel)
         << "\"";
    }

    os << ",\"user\":"   << t.spec.user;
    os << ",\"preds\":["; join(os, t.predecessors);
    os << "],\"succ\":[";  join(os, t.successors);
    os << "],\"deps\":[";

    for (int i = 0; i < t.spec.n_depends; i++) {
      const Depend& d = t.spec.dependencies[i];
      os << (i ? "," : "") << "{\"block\":" << d.id << ",\"mode\":\""
         << (d.mode == Mut ? "mut" : "ref") << "\"}";
    }
    os << "]";

    if (auto it = done.find(t.spec.id); it != done.end()) {
      const TaskTrace& tr = *it->second;
      os << ",\"trace\":{\"enqueue_ts\":" << tr.enqueue_ts;
      os << ",\"pop_ts\":"   << tr.pop_ts;
      os << ",\"start_ts\":" << tr.start_ts;
      os << ",\"end_ts\":"   << tr.end_ts;
      os << ",\"status\":"   << tr.status << "}";
    }
    os << "}\n";
  }
  os << "{\"type\":\"summary\",\"tasks\":" << vertices.size()
     << ",\"completed\":" << n_completed << "}\n";
}

uint32_t LocalDAG::infer_edges(TaskVertex& task) {
  auto add_pred_dedup = [&](TaskID pred) {
    if (!pred || pred == task.spec.id)
      return;
    auto& ps = task.predecessors;
    if (std::find(ps.begin(), ps.end(), pred) != ps.end())
      return;
    ps.push_back(pred);
    vertices[pred - 1].successors.push_back(task.spec.id);
  };

  for (size_t i = 0; i < task.spec.n_depends; i++) {
    const Depend& dep = task.spec.dependencies[i];
    BlockCursor& block = frontier[dep.id];

    add_pred_dedup(block.producer);
    if (dep.mode == Mut) {
      for (TaskID cons : block.consumers)
        add_pred_dedup(cons);
    }
  }

  for (size_t i = 0; i < task.spec.n_depends; i++) {
    const Depend& dep = task.spec.dependencies[i];
    BlockCursor& block = frontier[dep.id];

    if (dep.mode == Mut) {
      block.consumers.clear();
      block.producer = task.spec.id;
    } else {
      block.consumers.push_back(task.spec.id);
    }
  }

  return (uint32_t)task.predecessors.size();
}
