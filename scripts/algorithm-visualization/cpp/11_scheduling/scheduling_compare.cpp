// Confronto fra i quattro scheduler
//
// Gli stessi quattro processi del libro, dati in pasto ai quattro algoritmi. È
// il confronto a dare senso ai numeri: presa da sola, un'attesa media di 5.75
// non dice niente.
//
// Le implementazioni qui sono riscritte in forma compatta e senza
// instrumentazione. Sono volutamente indipendenti da quelle tracciate negli
// altri quattro file: se i due gruppi dessero risultati diversi, uno dei due
// sarebbe sbagliato, e il controllo automatico se ne accorge.
#include <algorithm>
#include <climits>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/scheduling.hpp"

static Tracer T("scheduling_compare", "Confronto fra i quattro scheduler", "Scheduling", __FILE__);

using trace::Metrics;
using trace::Process;
using trace::Slice;

struct Result {
    std::vector<Slice> gantt;
    std::map<std::string, Metrics> metrics;
    double avgWait = 0;
    double avgTurn = 0;
};

static void finish(Result& r, int n) {
    int w = 0, t = 0;
    for (const auto& kv : r.metrics) {
        w += kv.second.wait;
        t += kv.second.turn;
    }
    r.avgWait = static_cast<double>(w) / n;
    r.avgTurn = static_cast<double>(t) / n;
}

// Non preemptivo, scelta per arrivo / burst / priorità a seconda del criterio.
enum Criterion { ARRIVAL, BURST, PRIORITY };

static Result nonPreemptive(std::vector<Process> p, Criterion by) {
    const int n = static_cast<int>(p.size());
    std::vector<bool> done(n, false);
    Result r;
    int time = 0, completed = 0;

    while (completed < n) {
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (done[i] || p[i].arrival > time) continue;
            if (best < 0) { best = i; continue; }
            if (by == ARRIVAL && p[i].arrival < p[best].arrival) best = i;
            if (by == BURST && p[i].burst < p[best].burst) best = i;
            if (by == PRIORITY && p[i].priority < p[best].priority) best = i;
        }
        if (best < 0) {
            int next = INT_MAX;
            for (int i = 0; i < n; i++) if (!done[i]) next = std::min(next, p[i].arrival);
            r.gantt.push_back({"", time, next, true});
            time = next;
            continue;
        }
        int start = time;
        time += p[best].burst;
        done[best] = true;
        completed++;
        r.gantt.push_back({p[best].name, start, time, false});
        r.metrics[p[best].name] = {start - p[best].arrival, time - p[best].arrival};
    }
    finish(r, n);
    return r;
}

static Result roundRobin(std::vector<Process> p, int quantum) {
    const int n = static_cast<int>(p.size());
    std::vector<int> remaining(n);
    for (int i = 0; i < n; i++) remaining[i] = p[i].burst;
    std::vector<bool> queued(n, false);
    std::vector<int> queue;
    Result r;
    int time = 0, completed = 0;

    while (completed < n) {
        for (int i = 0; i < n; i++)
            if (!queued[i] && remaining[i] > 0 && p[i].arrival <= time) { queued[i] = true; queue.push_back(i); }
        if (queue.empty()) { r.gantt.push_back({"", time, time + 1, true}); time++; continue; }

        int cur = queue.front();
        queue.erase(queue.begin());
        int slice = std::min(remaining[cur], quantum);
        int start = time;
        time += slice;
        remaining[cur] -= slice;
        r.gantt.push_back({p[cur].name, start, time, false});

        for (int i = 0; i < n; i++)
            if (!queued[i] && remaining[i] > 0 && p[i].arrival <= time) { queued[i] = true; queue.push_back(i); }
        if (remaining[cur] == 0) {
            completed++;
            r.metrics[p[cur].name] = {time - p[cur].arrival - p[cur].burst, time - p[cur].arrival};
        } else {
            queue.push_back(cur);
        }
    }
    finish(r, n);
    return r;
}

// tracer-hide-begin
static std::string fmt(double v) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%.2f", v);
    return buf;
}

static std::vector<Process> procs;
static std::map<std::string, Result> results;
static const char* ORDER[] = {"FCFS", "SJF", "Priority", "Round Robin"};

static std::string rowJson(bool wait) {
    std::string s = "{";
    bool first = true;
    for (const char* name : ORDER) {
        auto it = results.find(name);
        if (it == results.end()) continue;
        if (!first) s += ",";
        first = false;
        s += trace::Val::quote(name) + ":" +
             trace::Val::quote(fmt(wait ? it->second.avgWait : it->second.avgTurn));
    }
    return s + "}";
}

static std::string ganttOf(const char* name) {
    auto it = results.find(name);
    if (it == results.end()) return trace::ganttJson(procs, {}, 0, {}, {}, false);
    return trace::ganttJson(procs, it->second.gantt, -1, it->second.metrics, {},
                            std::string(name) == "Priority");
}
// tracer-hide-end

int main() {
    T.view("table").complexity("—", "—");
    T.panel("gantt", "FCFS", "fcfs");
    T.panel("gantt", "SJF (non preemptive)", "sjf");
    T.panel("gantt", "Priority", "priority");
    T.panel("gantt", "Round Robin (q = 3)", "rr");

    procs = trace::bookProcesses();
    T.input("dati del libro");

    T.at(__LINE__, "main")
     .tableRaw("attesa media", rowJson(true))
     .tableRaw("turnaround medio", rowJson(false))
     .gantt("fcfs", ganttOf("FCFS")).gantt("sjf", ganttOf("SJF"))
     .gantt("priority", ganttOf("Priority")).gantt("rr", ganttOf("Round Robin"))
     .note("Gli stessi quattro processi del libro: P0(0,5), P1(1,3), P2(2,8), P3(3,6). "
           "Aggiungo uno scheduler alla volta.")
     .emit();

    results["FCFS"] = nonPreemptive(procs, ARRIVAL);
    T.at(__LINE__, "main")
     .vars({{"attesa media", fmt(results["FCFS"].avgWait)}})
     .tableRaw("attesa media", rowJson(true))
     .tableRaw("turnaround medio", rowJson(false))
     .gantt("fcfs", ganttOf("FCFS")).gantt("sjf", ganttOf("SJF"))
     .gantt("priority", ganttOf("Priority")).gantt("rr", ganttOf("Round Robin"))
     .note("FCFS: nessuna scelta, solo l'ordine di arrivo. È il termine di paragone.")
     .emit();

    results["SJF"] = nonPreemptive(procs, BURST);
    T.at(__LINE__, "main")
     .vars({{"attesa media", fmt(results["SJF"].avgWait)}})
     .tableRaw("attesa media", rowJson(true))
     .tableRaw("turnaround medio", rowJson(false))
     .gantt("fcfs", ganttOf("FCFS")).gantt("sjf", ganttOf("SJF"))
     .gantt("priority", ganttOf("Priority")).gantt("rr", ganttOf("Round Robin"))
     .note("SJF migliora l'attesa media rispetto a FCFS, e nessuno può fare meglio: è ottimo. "
           "In cambio serve conoscere in anticipo la durata dei processi.")
     .emit();

    results["Priority"] = nonPreemptive(procs, PRIORITY);
    T.at(__LINE__, "main")
     .vars({{"attesa media", fmt(results["Priority"].avgWait)}})
     .tableRaw("attesa media", rowJson(true))
     .tableRaw("turnaround medio", rowJson(false))
     .gantt("fcfs", ganttOf("FCFS")).gantt("sjf", ganttOf("SJF"))
     .gantt("priority", ganttOf("Priority")).gantt("rr", ganttOf("Round Robin"))
     .note("Su questi dati Priority produce lo stesso ordine di SJF, ma per una ragione diversa: "
           "non perché quei processi siano corti, ma perché hanno il numero di priorità più basso.")
     .emit();

    results["Round Robin"] = roundRobin(procs, trace::QUANTUM);
    T.at(__LINE__, "main")
     .vars({{"attesa media", fmt(results["Round Robin"].avgWait)}})
     .tableRaw("attesa media", rowJson(true))
     .tableRaw("turnaround medio", rowJson(false))
     .gantt("fcfs", ganttOf("FCFS")).gantt("sjf", ganttOf("SJF"))
     .gantt("priority", ganttOf("Priority")).gantt("rr", ganttOf("Round Robin"))
     .note("Round Robin è il peggiore per attesa media, e va benissimo così: il suo obiettivo è "
           "l'equità e un tempo di risposta basso per tutti, non la media. Confronta i quattro "
           "diagrammi: FCFS e SJF danno blocchi lunghi e continui, il round robin li spezzetta.")
     .emit();

    T.dump("traces/scheduling_compare.js");
    return 0;
}
