// First Come First Served
//
// Lo scheduler più semplice possibile: i processi vengono serviti nell'ordine
// di arrivo, e una volta partiti girano fino alla fine. Non preemptivo, nessuna
// scelta da fare, nessuna struttura oltre a una coda.
//
// Il suo difetto ha un nome: CONVOY EFFECT. Se il primo processo è lungo, tutti
// gli altri aspettano dietro di lui anche se sarebbero brevissimi, e l'attesa
// media va alle stelle. Confronta i due input per vederlo.
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
     .counters({{"tempo", time}})
     .note("Processi ordinati per istante di arrivo. La CPU è libera, il diagramma è vuoto.")
     .emit();

    for (const Process& p : procs) {
        if (time < p.arrival) {
            gantt.push_back({"", time, p.arrival, true});
            T.at(__LINE__, __func__)
             .vars({{"tempo", time}, {"prossimo", p.name}})
             .gantt(trace::ganttJson(procs, gantt, p.arrival, metrics, {}, false))
             .counters({{"tempo", p.arrival}})
             .note("Nessun processo è ancora arrivato: la CPU resta ferma fino a t = " +
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
         .vars({{"processo", p.name}, {"start", start}, {"fine", time},
                {"attesa", wait}, {"turnaround", turn}})
         .gantt(trace::ganttJson(procs, gantt, time, metrics, {}, false))
         .counters({{"tempo", time}})
         .note(p.name + " occupa la CPU da " + std::to_string(start) + " a " + std::to_string(time) +
               " senza interruzioni. Attesa = inizio - arrivo = " + std::to_string(start) + " - " +
               std::to_string(p.arrival) + " = " + std::to_string(wait) +
               "; turnaround = attesa + burst = " + std::to_string(turn) + ".")
         .emit();
    }

    int totWait = 0, totTurn = 0;
    for (const auto& kv : metrics) {
        totWait += kv.second.wait;
        totTurn += kv.second.turn;
    }
    T.at(__LINE__, __func__)
     .vars({{"attesa media", (double)totWait / procs.size()},
            {"turnaround medio", (double)totTurn / procs.size()}})
     .gantt(trace::ganttJson(procs, gantt, time, metrics, {}, false))
     .counters({{"tempo", time}})
     .note("Tutti terminati. Attesa media " + std::to_string((double)totWait / procs.size()).substr(0, 5) +
           ", turnaround medio " + std::to_string((double)totTurn / procs.size()).substr(0, 5) +
           ". Sono questi due numeri a permettere il confronto con gli altri tre scheduler.")
     .emit();
}

int main() {
    T.view("gantt").complexity("O(n log n) per l'ordinamento", "O(n)");

    T.input("dati del libro");
    fcfs(trace::bookProcesses());

    T.input("convoy effect (processo lungo per primo)");
    fcfs(trace::convoyProcesses());

    T.dump("traces/fcfs.js");
    return 0;
}
