// Shortest Job First
//
// Fra i processi già arrivati sceglie quello con il burst più corto. È
// dimostrabilmente ottimo per l'attesa media — nessun altro scheduler fa
// meglio — ma richiede di conoscere in anticipo la durata di ogni processo,
// che nella realtà non si sa. E chi è lungo rischia la starvation.
//
// Due varianti, due input:
//   non preemptive : chi parte finisce
//   preemptive (SRTF, Shortest Remaining Time First) : l'arrivo di un processo
//   più corto di quello che resta al processo in esecuzione lo interrompe
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
     .counters({{"tempo", 0}, {"completati", 0}})
     .note("SJF non preemptivo: a ogni scelta guardo chi è già arrivato e prendo il burst più corto. "
           "Una volta partito, nessuno lo interrompe.")
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
             .vars({{"tempo", time}})
             .gantt(trace::ganttJson(procs, gantt, next, metrics, {}, false))
             .counters({{"tempo", next}, {"completati", completed}})
             .note("Nessun processo arrivato: la CPU resta ferma fino a t = " + std::to_string(next) + ".")
             .emit();
            time = next;
            continue;
        }

        T.at(__LINE__, __func__)
         .vars({{"tempo", time}, {"scelto", procs[best].name}, {"burst", procs[best].burst}})
         .gantt(trace::ganttJson(procs, gantt, time, metrics,
                                 readyNames(procs, remaining, time, best), false))
         .counters({{"tempo", time}, {"completati", completed}})
         .note("A t = " + std::to_string(time) + " scelgo " + procs[best].name + ", burst " +
               std::to_string(procs[best].burst) + ": è il più corto fra quelli pronti.")
         .emit();

        int start = time;
        time += procs[best].burst;
        remaining[best] = 0;
        completed++;
        gantt.push_back({procs[best].name, start, time, false});
        metrics[procs[best].name] = {start - procs[best].arrival, time - procs[best].arrival};

        T.at(__LINE__, __func__)
         .vars({{"processo", procs[best].name}, {"fine", time},
                {"attesa", start - procs[best].arrival}})
         .gantt(trace::ganttJson(procs, gantt, time, metrics,
                                 readyNames(procs, remaining, time, -1), false))
         .counters({{"tempo", time}, {"completati", completed}})
         .note(procs[best].name + " gira fino in fondo, da " + std::to_string(start) + " a " +
               std::to_string(time) + ". Nel frattempo la coda dei pronti si è riempita e si "
               "riordinerà alla prossima scelta.")
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
     .counters({{"tempo", 0}, {"completati", 0}})
     .note("SRTF: la scelta si rifà a ogni istante, guardando il tempo RIMANENTE. Un processo corto "
           "che arriva può interrompere quello in esecuzione.")
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
                 .vars({{"tempo", time}, {"interrotto", procs[running].name},
                        {"rimanente", remaining[running]}, {"subentra", procs[best].name},
                        {"rimanente subentrante", remaining[best]}})
                 .gantt(trace::ganttJson(procs, gantt, time, metrics,
                                         readyNames(procs, remaining, time, best), false))
                 .counters({{"tempo", time}, {"completati", completed}})
                 .note("PRELAZIONE a t = " + std::to_string(time) + ": a " + procs[running].name +
                       " restano " + std::to_string(remaining[running]) + " unità, ma a " +
                       procs[best].name + " ne bastano " + std::to_string(remaining[best]) +
                       ". Il primo torna in coda con il suo residuo, non riparte da capo.")
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
             .vars({{"processo", procs[best].name}, {"fine", time},
                    {"turnaround", time - procs[best].arrival}})
             .gantt(trace::ganttJson(procs, gantt, time, metrics,
                                     readyNames(procs, remaining, time, -1), false))
             .counters({{"tempo", time}, {"completati", completed}})
             .note(procs[best].name + " termina a t = " + std::to_string(time) +
                   ". Turnaround = fine - arrivo = " + std::to_string(time - procs[best].arrival) +
                   ", attesa = turnaround - burst = " +
                   std::to_string(time - procs[best].arrival - procs[best].burst) + ".")
             .emit();
            running = -1;
        }
    }
}

int main() {
    T.view("gantt").complexity("O(n²) con scansione lineare", "O(n)");

    T.input("non preemptive (dati del libro)");
    sjfNonPreemptive(trace::bookProcesses());

    T.input("preemptive / SRTF (dati del libro)");
    sjfPreemptive(trace::bookProcesses());

    T.input("non preemptive (convoy)");
    sjfNonPreemptive(trace::convoyProcesses());

    T.dump("traces/sjf.js");
    return 0;
}
