// threads.hpp - the process model for the synchronization chapter.
//
// No real threads. Each process is a state machine that can be advanced one
// step at a time, and an explicit scheduler decides whose turn is next. That
// way the interleaving is deterministic, reproducible and — above all —
// CHOOSABLE: it's the only reliable way to show a concurrency bug, instead
// of hoping it shows up.
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "tracer.hpp"

namespace trace {

struct ThreadProc {
    std::string name;
    std::vector<std::string> lines;
    int pc = 0;
    bool blocked = false;
    bool critical = false;
    bool done = false;
};

using Shared = std::vector<std::pair<std::string, std::string>>;

// withLines: only on the first step. The processes' code never changes, and
// repeating it every step would bloat the trace for nothing.
// maxInCritical: how many processes can legitimately be inside together.
// For mutual exclusion it's 1, and exceeding it is a violation; for a
// counting semaphore with 5 printers it's 5, and five processes inside is normal.
inline std::string threadsJson(const std::vector<ThreadProc>& procs, int running,
                               const Shared& shared,
                               const std::vector<std::string>& blocked,
                               bool withLines, int maxInCritical = 1) {
    std::string s = "{\"procs\":[";
    for (size_t i = 0; i < procs.size(); ++i) {
        if (i) s += ",";
        const ThreadProc& p = procs[i];
        const char* state = p.done ? "done"
                          : p.blocked ? "blocked"
                          : ((int)i == running ? "running" : "ready");
        s += "{\"name\":" + Val::quote(p.name) +
             ",\"pc\":" + std::to_string(p.pc) +
             ",\"state\":" + Val::quote(state) +
             ",\"critical\":" + std::string(p.critical ? "true" : "false");
        if (withLines) {
            s += ",\"lines\":[";
            for (size_t k = 0; k < p.lines.size(); ++k) {
                if (k) s += ",";
                s += Val::quote(p.lines[k]);
            }
            s += "]";
        }
        s += "}";
    }
    s += "],\"shared\":{";
    for (size_t i = 0; i < shared.size(); ++i) {
        if (i) s += ",";
        s += Val::quote(shared[i].first) + ":" + Val::quote(shared[i].second);
    }
    s += "},\"blocked\":[";
    for (size_t i = 0; i < blocked.size(); ++i) {
        if (i) s += ",";
        s += Val::quote(blocked[i]);
    }
    s += "],\"maxInCritical\":" + std::to_string(maxInCritical);
    return s + "}";
}

}  // namespace trace
