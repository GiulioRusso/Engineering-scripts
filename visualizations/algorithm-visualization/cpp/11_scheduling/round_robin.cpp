// Round Robin
//
// Every process is given a time QUANTUM; once the quantum expires, if the
// process isn't finished it's preempted and put back at the TAIL of the
// ready queue. It's FCFS with a timer, and the timer is the algorithm.
//
// The quantum decides everything: too large, and round robin degenerates
// into FCFS; too small, and the CPU spends its time context-switching
// instead of working. Round robin doesn't minimize average wait — on this
// data it's the worst of the four — but it's the only fair one: nobody
// waits indefinitely, and RESPONSE time is low for everyone.
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/scheduling.hpp"

static Tracer T("round_robin", "Round Robin", "Scheduling", __FILE__);

using trace::Metrics;
using trace::Process;
using trace::Slice;

// tracer-hide-begin
static std::vector<std::string> queueNames(const std::vector<Process>& p,
                                           const std::vector<int>& queue) {
    std::vector<std::string> out;
    for (int i : queue) out.push_back(p[i].name);
    return out;
}
// tracer-hide-end

void roundRobin(std::vector<Process> procs, int quantum) {
    const int n = static_cast<int>(procs.size());
    std::vector<int> remaining(n);
    for (int i = 0; i < n; i++) remaining[i] = procs[i].burst;

    std::vector<bool> queued(n, false);
    std::vector<int> queue;
    std::vector<Slice> gantt;
    std::map<std::string, Metrics> metrics;
    int time = 0;
    int completed = 0;
    int switches = 0;

    T.at(__LINE__, __func__)
     .vars({{"quantum", quantum}})
     .gantt(trace::ganttJson(procs, gantt, time, metrics, {}, false))
     .counters({{"time", 0}, {"preemptions", 0}})
     .note("Quantum = " + std::to_string(quantum) +
           ". The ready queue is empty; processes join it as they arrive.")
     .emit();

    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (!queued[i] && remaining[i] > 0 && procs[i].arrival <= time) {
                queued[i] = true;
                queue.push_back(i);
            }
        }

        if (queue.empty()) {
            gantt.push_back({"", time, time + 1, true});
            time++;
            continue;
        }

        int current = queue.front();
        queue.erase(queue.begin());
        int slice = remaining[current] < quantum ? remaining[current] : quantum;

        T.at(__LINE__, __func__)
         .vars({{"time", time}, {"on CPU", procs[current].name},
                {"remaining", remaining[current]}, {"quantum", quantum}})
         .gantt(trace::ganttJson(procs, gantt, time, metrics, queueNames(procs, queue), false))
         .counters({{"time", time}, {"preemptions", switches}})
         .note(procs[current].name + " takes the CPU at t = " + std::to_string(time) +
               ". The timer starts at " + std::to_string(quantum) + " and it has " +
               std::to_string(remaining[current]) + " units left: it will run for " + std::to_string(slice) + ".")
         .emit();

        int start = time;
        time += slice;
        remaining[current] -= slice;
        gantt.push_back({procs[current].name, start, time, false});

        // Whoever arrives DURING the quantum joins the queue before the process
        // that's just been preempted: that's what costs it its turn.
        for (int i = 0; i < n; i++) {
            if (!queued[i] && remaining[i] > 0 && procs[i].arrival <= time) {
                queued[i] = true;
                queue.push_back(i);
            }
        }

        if (remaining[current] == 0) {
            completed++;
            metrics[procs[current].name] = {time - procs[current].arrival - procs[current].burst,
                                            time - procs[current].arrival};
            T.at(__LINE__, __func__)
             .vars({{"process", procs[current].name}, {"end", time}})
             .gantt(trace::ganttJson(procs, gantt, time, metrics, queueNames(procs, queue), false))
             .counters({{"time", time}, {"preemptions", switches}})
             .note(procs[current].name + " finished before the quantum expired: it leaves the system at "
                   "t = " + std::to_string(time) + " and doesn't return to the queue.")
             .emit();
        } else {
            switches++;
            queue.push_back(current);
            T.at(__LINE__, __func__)
             .vars({{"process", procs[current].name}, {"remaining", remaining[current]}})
             .gantt(trace::ganttJson(procs, gantt, time, metrics, queueNames(procs, queue), false))
             .counters({{"time", time}, {"preemptions", switches}})
             .note("QUANTUM EXPIRED at t = " + std::to_string(time) + ": " + procs[current].name +
                   " is preempted with " + std::to_string(remaining[current]) +
                   " units still to go and goes back to the TAIL of the queue, behind everyone who "
                   "arrived in the meantime. It resumes from its remainder, not from scratch.")
             .emit();
        }
    }

    int totWait = 0, totTurn = 0;
    for (const auto& kv : metrics) {
        totWait += kv.second.wait;
        totTurn += kv.second.turn;
    }
    T.at(__LINE__, __func__)
     .vars({{"avg wait", (double)totWait / n}, {"avg turnaround", (double)totTurn / n},
            {"preemptions", switches}})
     .gantt(trace::ganttJson(procs, gantt, time, metrics, {}, false))
     .counters({{"time", time}, {"preemptions", switches}})
     .note("Done, with " + std::to_string(switches) +
           " preemptions. The average wait is the worst of the four schedulers, but no process "
           "sat idle for long: round robin optimizes for fairness, not the average.")
     .emit();
}

int main() {
    T.view("gantt").complexity("O(n) per preemption", "O(n)");

    T.input("quantum = 3 (book data)");
    roundRobin(trace::bookProcesses(), trace::QUANTUM);

    T.input("quantum = 1 (many preemptions)");
    roundRobin(trace::bookProcesses(), 1);

    T.input("quantum = 10 (degenerates into FCFS)");
    roundRobin(trace::bookProcesses(), 10);

    T.dump("traces/round_robin.js");
    return 0;
}
