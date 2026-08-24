// Shortest Job First
//
// Among the processes that have already arrived, picks the one with the
// shortest burst. Provably optimal for average wait — no other scheduler
// does better — but it needs to know each process's duration in advance,
// which in reality isn't known. And a long process risks starvation.
//
// Two variants, two inputs:
//   non-preemptive: whoever starts finishes
//   preemptive (SRTF, Shortest Remaining Time First): a process arriving
//   with less work left than the one currently running preempts it
#include <algorithm>
#include <climits>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/scheduling.hpp"

static Tracer T("sjf", "Shortest Job First", "Scheduling", __FILE__);

using trace::Metrics;
using trace::Process;
using trace::Slice;

// tracer-hide-begin
static std::vector<std::string> readyNames(const std::vector<Process>& p,
                                           const std::vector<int>& remaining, int time, int running) {
    std::vector<std::string> out;
    for (size_t i = 0; i < p.size(); i++)
        if ((int)i != running && remaining[i] > 0 && p[i].arrival <= time) out.push_back(p[i].name);
    return out;
}
// tracer-hide-end

void sjfNonPreemptive(std::vector<Process> procs) {
    const int n = static_cast<int>(procs.size());
    std::vector<int> remaining(n);
    for (int i = 0; i < n; i++) remaining[i] = procs[i].burst;

    std::vector<Slice> gantt;
    std::map<std::string, Metrics> metrics;
    int time = 0;
    int completed = 0;

    T.at(__LINE__, __func__)
     .gantt(trace::ganttJson(procs, gantt, time, metrics, {}, false))
     .counters({{"time", 0}, {"completed", 0}})
     .note("Non-preemptive SJF: at every choice, look at who has already arrived and take the "
           "shortest burst. Once started, nothing interrupts it.")
     .emit();

    while (completed < n) {
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (remaining[i] > 0 && procs[i].arrival <= time) {
                if (best < 0 || procs[i].burst < procs[best].burst) best = i;
            }
        }

        if (best < 0) {
            int next = INT_MAX;
            for (int i = 0; i < n; i++)
                if (remaining[i] > 0) next = std::min(next, procs[i].arrival);
            gantt.push_back({"", time, next, true});
            T.at(__LINE__, __func__)
             .vars({{"time", time}})
             .gantt(trace::ganttJson(procs, gantt, next, metrics, {}, false))
             .counters({{"time", next}, {"completed", completed}})
             .note("No process has arrived: the CPU stays idle until t = " + std::to_string(next) + ".")
             .emit();
            time = next;
            continue;
        }

        T.at(__LINE__, __func__)
         .vars({{"time", time}, {"chosen", procs[best].name}, {"burst", procs[best].burst}})
         .gantt(trace::ganttJson(procs, gantt, time, metrics,
                                 readyNames(procs, remaining, time, best), false))
         .counters({{"time", time}, {"completed", completed}})
         .note("At t = " + std::to_string(time) + " pick " + procs[best].name + ", burst " +
               std::to_string(procs[best].burst) + ": it's the shortest among the ready ones.")
         .emit();

        int start = time;
        time += procs[best].burst;
        remaining[best] = 0;
        completed++;
        gantt.push_back({procs[best].name, start, time, false});
        metrics[procs[best].name] = {start - procs[best].arrival, time - procs[best].arrival};

        T.at(__LINE__, __func__)
         .vars({{"process", procs[best].name}, {"end", time},
                {"wait", start - procs[best].arrival}})
         .gantt(trace::ganttJson(procs, gantt, time, metrics,
                                 readyNames(procs, remaining, time, -1), false))
         .counters({{"time", time}, {"completed", completed}})
         .note(procs[best].name + " runs to completion, from " + std::to_string(start) + " to " +
               std::to_string(time) + ". Meanwhile the ready queue has filled up and will be "
               "reordered at the next choice.")
         .emit();
    }
}

void sjfPreemptive(std::vector<Process> procs) {
    const int n = static_cast<int>(procs.size());
    std::vector<int> remaining(n);
    for (int i = 0; i < n; i++) remaining[i] = procs[i].burst;

    std::vector<Slice> gantt;
    std::map<std::string, Metrics> metrics;
    int time = 0;
    int completed = 0;
    int running = -1;
    int sliceStart = 0;

    T.at(__LINE__, __func__)
     .gantt(trace::ganttJson(procs, gantt, time, metrics, {}, false))
     .counters({{"time", 0}, {"completed", 0}})
     .note("SRTF: the choice is redone at every instant, looking at the REMAINING time. A short "
           "process that arrives can preempt the one running.")
     .emit();

    while (completed < n) {
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (remaining[i] > 0 && procs[i].arrival <= time) {
                if (best < 0 || remaining[i] < remaining[best]) best = i;
            }
        }

        if (best < 0) {
            gantt.push_back({"", time, time + 1, true});
            time++;
            continue;
        }

        if (best != running) {
            if (running >= 0) {
                gantt.push_back({procs[running].name, sliceStart, time, false});
                T.at(__LINE__, __func__)
                 .vars({{"time", time}, {"preempted", procs[running].name},
                        {"remaining", remaining[running]}, {"next up", procs[best].name},
                        {"next up remaining", remaining[best]}})
                 .gantt(trace::ganttJson(procs, gantt, time, metrics,
                                         readyNames(procs, remaining, time, best), false))
                 .counters({{"time", time}, {"completed", completed}})
                 .note("PREEMPTION at t = " + std::to_string(time) + ": " + procs[running].name +
                       " has " + std::to_string(remaining[running]) + " units left, but " +
                       procs[best].name + " only needs " + std::to_string(remaining[best]) +
                       ". The first goes back to the queue with its remainder, it doesn't restart from scratch.")
                 .emit();
            }
            running = best;
            sliceStart = time;
        }

        remaining[best]--;
        time++;

        if (remaining[best] == 0) {
            completed++;
            gantt.push_back({procs[best].name, sliceStart, time, false});
            metrics[procs[best].name] = {time - procs[best].arrival - procs[best].burst,
                                         time - procs[best].arrival};
            T.at(__LINE__, __func__)
             .vars({{"process", procs[best].name}, {"end", time},
                    {"turnaround", time - procs[best].arrival}})
             .gantt(trace::ganttJson(procs, gantt, time, metrics,
                                     readyNames(procs, remaining, time, -1), false))
             .counters({{"time", time}, {"completed", completed}})
             .note(procs[best].name + " finishes at t = " + std::to_string(time) +
                   ". Turnaround = end - arrival = " + std::to_string(time - procs[best].arrival) +
                   ", wait = turnaround - burst = " +
                   std::to_string(time - procs[best].arrival - procs[best].burst) + ".")
             .emit();
            running = -1;
        }
    }
}

int main() {
    T.view("gantt").complexity("O(n²) with a linear scan", "O(n)");

    T.input("non-preemptive (book data)");
    sjfNonPreemptive(trace::bookProcesses());

    T.input("preemptive / SRTF (book data)");
    sjfPreemptive(trace::bookProcesses());

    T.input("non-preemptive (convoy)");
    sjfNonPreemptive(trace::convoyProcesses());

    T.dump("traces/sjf.js");
    return 0;
}
