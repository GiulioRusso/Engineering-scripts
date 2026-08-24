// Priority Scheduling
//
// Every process carries a priority number; among those that have arrived,
// the one with the highest priority runs. Here, as in the book, a LOWER
// number means a higher priority.
//
// The problem is STARVATION: a low-priority process can sit idle
// indefinitely, because more important processes just keep arriving. In the
// book's data P2 has priority 5, the worst, and indeed it runs last despite
// arriving third. The usual fix is AGING: raise the priority of whoever has
// been waiting too long.
#include <algorithm>
#include <climits>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/scheduling.hpp"

static Tracer T("priority_scheduling", "Priority Scheduling", "Scheduling", __FILE__);

using trace::Metrics;
using trace::Process;
using trace::Slice;

// tracer-hide-begin
static std::vector<std::string> readyNames(const std::vector<Process>& p,
                                           const std::vector<bool>& done, int time, int running) {
    std::vector<std::string> out;
    for (size_t i = 0; i < p.size(); i++)
        if ((int)i != running && !done[i] && p[i].arrival <= time) out.push_back(p[i].name);
    return out;
}
// tracer-hide-end

void priorityScheduling(std::vector<Process> procs) {
    const int n = static_cast<int>(procs.size());
    std::vector<bool> done(n, false);
    std::vector<Slice> gantt;
    std::map<std::string, Metrics> metrics;
    int time = 0;
    int completed = 0;

    T.at(__LINE__, __func__)
     .gantt(trace::ganttJson(procs, gantt, time, metrics, {}, true))
     .counters({{"time", 0}, {"completed", 0}})
     .note("The table now has a priority column. Lower number = higher priority.")
     .emit();

    while (completed < n) {
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (!done[i] && procs[i].arrival <= time) {
                if (best < 0 || procs[i].priority < procs[best].priority) best = i;
            }
        }

        if (best < 0) {
            int next = INT_MAX;
            for (int i = 0; i < n; i++)
                if (!done[i]) next = std::min(next, procs[i].arrival);
            gantt.push_back({"", time, next, true});
            time = next;
            continue;
        }

        T.at(__LINE__, __func__)
         .vars({{"time", time}, {"chosen", procs[best].name}, {"priority", procs[best].priority}})
         .gantt(trace::ganttJson(procs, gantt, time, metrics,
                                 readyNames(procs, done, time, best), true))
         .counters({{"time", time}, {"completed", completed}})
         .note("At t = " + std::to_string(time) + " the ready process with the best priority is " +
               procs[best].name + " (priority " + std::to_string(procs[best].priority) +
               "). Duration doesn't matter: only the number does.")
         .emit();

        int start = time;
        time += procs[best].burst;
        done[best] = true;
        completed++;
        gantt.push_back({procs[best].name, start, time, false});
        metrics[procs[best].name] = {start - procs[best].arrival, time - procs[best].arrival};

        int worst = -1;
        for (int i = 0; i < n; i++)
            if (!done[i] && (worst < 0 || procs[i].priority > procs[worst].priority)) worst = i;

        T.at(__LINE__, __func__)
         .vars({{"process", procs[best].name}, {"end", time},
                {"wait", start - procs[best].arrival}})
         .gantt(trace::ganttJson(procs, gantt, time, metrics,
                                 readyNames(procs, done, time, -1), true))
         .counters({{"time", time}, {"completed", completed}})
         .note(procs[best].name + " finishes at " + std::to_string(time) + "." +
               (worst >= 0
                  ? " Meanwhile " + procs[worst].name + ", priority " +
                    std::to_string(procs[worst].priority) +
                    ", keeps waiting: if more important processes kept arriving it might never "
                    "run. That's starvation."
                  : " Nobody is left waiting."))
         .emit();
    }
}

int main() {
    T.view("gantt").complexity("O(n²) with a linear scan", "O(n)");

    T.input("book data");
    priorityScheduling(trace::bookProcesses());

    T.input("low-priority process starves");
    priorityScheduling({{"P0", 0, 4, 1}, {"P1", 1, 3, 1}, {"P2", 2, 2, 9}, {"P3", 3, 3, 1}});

    T.dump("traces/priority_scheduling.js");
    return 0;
}
