// Priority Scheduling
//
// Ogni processo porta un numero di priorità; fra quelli arrivati va in CPU
// quello con la priorità più alta. Qui, come nel libro, il numero più BASSO
// indica la priorità più alta.
//
// Il problema è la STARVATION: un processo a bassa priorità può restare fermo
// indefinitamente, perché basta che continuino ad arrivare processi più
// importanti. Nei dati del libro P2 ha priorità 5, la peggiore, e infatti va in
// CPU per ultimo pur essendo arrivato terzo. La soluzione usuale è l'AGING:
// alzare la priorità di chi aspetta da troppo tempo.
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
     .counters({{"tempo", 0}, {"completati", 0}})
     .note("La tabella ora ha la colonna priorità. Numero più basso = priorità più alta.")
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
         .vars({{"tempo", time}, {"scelto", procs[best].name}, {"priorità", procs[best].priority}})
         .gantt(trace::ganttJson(procs, gantt, time, metrics,
                                 readyNames(procs, done, time, best), true))
         .counters({{"tempo", time}, {"completati", completed}})
         .note("A t = " + std::to_string(time) + " il pronto con priorità migliore è " +
               procs[best].name + " (priorità " + std::to_string(procs[best].priority) +
               "). La durata non conta: conta solo il numero.")
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
         .vars({{"processo", procs[best].name}, {"fine", time},
                {"attesa", start - procs[best].arrival}})
         .gantt(trace::ganttJson(procs, gantt, time, metrics,
                                 readyNames(procs, done, time, -1), true))
         .counters({{"tempo", time}, {"completati", completed}})
         .note(procs[best].name + " finisce a " + std::to_string(time) + "." +
               (worst >= 0
                  ? " Intanto " + procs[worst].name + ", priorità " +
                    std::to_string(procs[worst].priority) +
                    ", continua ad aspettare: se arrivassero altri processi importanti potrebbe non "
                    "partire mai. È la starvation."
                  : " Non resta nessuno in attesa."))
         .emit();
    }
}

int main() {
    T.view("gantt").complexity("O(n²) con scansione lineare", "O(n)");

    T.input("dati del libro");
    priorityScheduling(trace::bookProcesses());

    T.input("starvation del processo a bassa priorità");
    priorityScheduling({{"P0", 0, 4, 1}, {"P1", 1, 3, 1}, {"P2", 2, 2, 9}, {"P3", 3, 3, 1}});

    T.dump("traces/priority_scheduling.js");
    return 0;
}
