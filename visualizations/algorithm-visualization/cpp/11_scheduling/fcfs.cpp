// First Come First Served
//
// The simplest scheduler possible: processes are served in arrival order,
// and once started they run to completion. Non-preemptive, no choice to
// make, no structure beyond a queue.
//
// Its flaw has a name: CONVOY EFFECT. If the first process is long, every
// other one waits behind it even if they're very short, and the average
// wait skyrockets. Compare the two inputs to see it.
#include <algorithm>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/scheduling.hpp"

static Tracer T("fcfs", "First Come First Served", "Scheduling", __FILE__);

using trace::Metrics;
using trace::Process;
using trace::Slice;

void fcfs(std::vector<Process> procs) {
    std::sort(procs.begin(), procs.end(),
              [](const Process& a, const Process& b) { return a.arrival < b.arrival; });

    std::vector<Slice> gantt;
    std::map<std::string, Metrics> metrics;
    int time = 0;
    T.at(__LINE__, __func__)
     .gantt(trace::ganttJson(procs, gantt, time, metrics, {}, false))
     .counters({{"time", time}})
     .note("Processes sorted by arrival time. The CPU is free, the chart is empty.")
     .emit();

    for (const Process& p : procs) {
        if (time < p.arrival) {
            gantt.push_back({"", time, p.arrival, true});
            T.at(__LINE__, __func__)
             .vars({{"time", time}, {"next", p.name}})
             .gantt(trace::ganttJson(procs, gantt, p.arrival, metrics, {}, false))
             .counters({{"time", p.arrival}})
             .note("No process has arrived yet: the CPU stays idle until t = " +
                   std::to_string(p.arrival) + ".")
             .emit();
            time = p.arrival;
        }

        int start = time;
        time += p.burst;
        gantt.push_back({p.name, start, time, false});

        int wait = start - p.arrival;
        int turn = time - p.arrival;
        metrics[p.name] = {wait, turn};

        T.at(__LINE__, __func__)
         .vars({{"process", p.name}, {"start", start}, {"end", time},
                {"wait", wait}, {"turnaround", turn}})
         .gantt(trace::ganttJson(procs, gantt, time, metrics, {}, false))
         .counters({{"time", time}})
         .note(p.name + " holds the CPU from " + std::to_string(start) + " to " + std::to_string(time) +
               " with no interruptions. Wait = start - arrival = " + std::to_string(start) + " - " +
               std::to_string(p.arrival) + " = " + std::to_string(wait) +
               "; turnaround = wait + burst = " + std::to_string(turn) + ".")
         .emit();
    }

    int totWait = 0, totTurn = 0;
    for (const auto& kv : metrics) {
        totWait += kv.second.wait;
        totTurn += kv.second.turn;
    }
    T.at(__LINE__, __func__)
     .vars({{"avg wait", (double)totWait / procs.size()},
            {"avg turnaround", (double)totTurn / procs.size()}})
     .gantt(trace::ganttJson(procs, gantt, time, metrics, {}, false))
     .counters({{"time", time}})
     .note("All done. Average wait " + std::to_string((double)totWait / procs.size()).substr(0, 5) +
           ", average turnaround " + std::to_string((double)totTurn / procs.size()).substr(0, 5) +
           ". These two numbers are what let it be compared with the other three schedulers.")
     .emit();
}

int main() {
    T.view("gantt").complexity("O(n log n) for the sort", "O(n)");

    T.input("book data");
    fcfs(trace::bookProcesses());

    T.input("convoy effect (long process first)");
    fcfs(trace::convoyProcesses());

    T.dump("traces/fcfs.js");
    return 0;
}
