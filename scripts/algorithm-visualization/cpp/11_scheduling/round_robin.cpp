// Round Robin
//
// A ogni processo si dà un QUANTO di tempo; scaduto il quanto, se il processo
// non ha finito viene prelazionato e rimesso IN FONDO alla coda dei pronti. È
// FCFS con un timer, e il timer è l'algoritmo.
//
// Il quanto decide tutto: molto grande, e il round robin degenera in FCFS;
// molto piccolo, e la CPU passa il tempo a cambiare contesto invece di
// lavorare. Il round robin non minimizza l'attesa media — su questi dati è il
// peggiore dei quattro — ma è l'unico equo: nessuno aspetta indefinitamente, e
// il tempo di RISPOSTA è basso per tutti.
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
     .vars({{"quanto", quantum}})
     .gantt(trace::ganttJson(procs, gantt, time, metrics, {}, false))
     .counters({{"tempo", 0}, {"prelazioni", 0}})
     .note("Quanto = " + std::to_string(quantum) +
           ". La coda dei pronti è vuota; i processi ci entrano man mano che arrivano.")
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
         .vars({{"tempo", time}, {"in CPU", procs[current].name},
                {"rimanente", remaining[current]}, {"quanto", quantum}})
         .gantt(trace::ganttJson(procs, gantt, time, metrics, queueNames(procs, queue), false))
         .counters({{"tempo", time}, {"prelazioni", switches}})
         .note(procs[current].name + " prende la CPU a t = " + std::to_string(time) +
               ". Il timer parte da " + std::to_string(quantum) + " e gli restano " +
               std::to_string(remaining[current]) + " unità: girerà per " + std::to_string(slice) + ".")
         .emit();

        int start = time;
        time += slice;
        remaining[current] -= slice;
        gantt.push_back({procs[current].name, start, time, false});

        // Chi arriva DURANTE il quanto entra in coda prima del processo appena
        // prelazionato: è quello che gli fa perdere il turno.
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
             .vars({{"processo", procs[current].name}, {"fine", time}})
             .gantt(trace::ganttJson(procs, gantt, time, metrics, queueNames(procs, queue), false))
             .counters({{"tempo", time}, {"prelazioni", switches}})
             .note(procs[current].name + " ha finito prima che il quanto scadesse: esce dal sistema a "
                   "t = " + std::to_string(time) + " e non torna in coda.")
             .emit();
        } else {
            switches++;
            queue.push_back(current);
            T.at(__LINE__, __func__)
             .vars({{"processo", procs[current].name}, {"rimanente", remaining[current]}})
             .gantt(trace::ganttJson(procs, gantt, time, metrics, queueNames(procs, queue), false))
             .counters({{"tempo", time}, {"prelazioni", switches}})
             .note("QUANTO SCADUTO a t = " + std::to_string(time) + ": " + procs[current].name +
                   " viene prelazionato con " + std::to_string(remaining[current]) +
                   " unità ancora da fare e torna IN FONDO alla coda, dietro a tutti quelli arrivati "
                   "nel frattempo. Riprenderà dal residuo, non da capo.")
             .emit();
        }
    }

    int totWait = 0, totTurn = 0;
    for (const auto& kv : metrics) {
        totWait += kv.second.wait;
        totTurn += kv.second.turn;
    }
    T.at(__LINE__, __func__)
     .vars({{"attesa media", (double)totWait / n}, {"turnaround medio", (double)totTurn / n},
            {"prelazioni", switches}})
     .gantt(trace::ganttJson(procs, gantt, time, metrics, {}, false))
     .counters({{"tempo", time}, {"prelazioni", switches}})
     .note("Finito, con " + std::to_string(switches) +
           " prelazioni. L'attesa media è la peggiore dei quattro scheduler, ma nessun processo è "
           "rimasto fermo a lungo: il round robin ottimizza l'equità, non la media.")
     .emit();
}

int main() {
    T.view("gantt").complexity("O(n) per prelazione", "O(n)");

    T.input("quanto = 3 (dati del libro)");
    roundRobin(trace::bookProcesses(), trace::QUANTUM);

    T.input("quanto = 1 (molte prelazioni)");
    roundRobin(trace::bookProcesses(), 1);

    T.input("quanto = 10 (degenera in FCFS)");
    roundRobin(trace::bookProcesses(), 10);

    T.dump("traces/round_robin.js");
    return 0;
}
