// scheduling.hpp - process data and the Gantt snapshot.
//
// Il libro per questo capitolo non dà codice, solo tabelle e diagrammi di
// Gantt. I dati dei processi qui sotto sono i suoi, così i risultati calcolati
// coincidono con le sue figure.
//
//   P0(arrivo 0, burst 5)   P1(1, 3)   P2(2, 8)   P3(3, 6)
//   priorità 1, 2, 5, 3 (numero più basso = priorità più alta)
//   quanto del round robin = 3
#pragma once

#include <map>
#include <string>
#include <vector>

#include "tracer.hpp"

namespace trace {

struct Process {
    std::string name;
    int arrival;
    int burst;
    int priority;
};

struct Metrics {
    int wait = -1;
    int turn = -1;
};

struct Slice {
    std::string proc;
    int start;
    int end;
    bool idle = false;
};

inline std::vector<Process> bookProcesses() {
    return {{"P0", 0, 5, 1}, {"P1", 1, 3, 2}, {"P2", 2, 8, 5}, {"P3", 3, 6, 3}};
}

// Un secondo set, con un processo lungo che arriva per primo: è il convoy
// effect, e su FCFS si vede a occhio nudo.
inline std::vector<Process> convoyProcesses() {
    return {{"P0", 0, 12, 3}, {"P1", 1, 2, 1}, {"P2", 2, 2, 2}, {"P3", 3, 2, 4}};
}

inline const int QUANTUM = 3;

inline std::string ganttJson(const std::vector<Process>& procs,
                             const std::vector<Slice>& slices,
                             int now,
                             const std::map<std::string, Metrics>& metrics,
                             const std::vector<std::string>& ready,
                             bool showPriority) {
    std::string s = "{\"processes\":[";
    for (size_t i = 0; i < procs.size(); ++i) {
        if (i) s += ",";
        s += "{\"name\":" + Val::quote(procs[i].name) +
             ",\"arrival\":" + std::to_string(procs[i].arrival) +
             ",\"burst\":" + std::to_string(procs[i].burst) +
             ",\"priority\":" + std::to_string(procs[i].priority) + "}";
    }
    s += "],\"slices\":[";
    for (size_t i = 0; i < slices.size(); ++i) {
        if (i) s += ",";
        s += "{\"proc\":" + Val::quote(slices[i].proc) +
             ",\"start\":" + std::to_string(slices[i].start) +
             ",\"end\":" + std::to_string(slices[i].end) +
             ",\"idle\":" + std::string(slices[i].idle ? "true" : "false") + "}";
    }
    s += "],\"metrics\":{";
    bool first = true;
    for (const auto& kv : metrics) {
        if (kv.second.wait < 0) continue;
        if (!first) s += ",";
        first = false;
        s += Val::quote(kv.first) + ":{\"wait\":" + std::to_string(kv.second.wait) +
             ",\"turn\":" + std::to_string(kv.second.turn) + "}";
    }
    s += "},\"ready\":[";
    for (size_t i = 0; i < ready.size(); ++i) {
        if (i) s += ",";
        s += Val::quote(ready[i]);
    }
    s += "],\"now\":" + std::to_string(now);
    s += ",\"showPriority\":" + std::string(showPriority ? "true" : "false") + "}";
    return s;
}

}  // namespace trace
