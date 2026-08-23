// Semafori
//
// Un semaforo è un intero con due sole operazioni, entrambe atomiche:
//
//     wait(S):    S--;  se S < 0  →  il processo si BLOCCA e va in coda
//     signal(S):  S++;  se S <= 0 →  sveglia un processo dalla coda
//
// Due cose da guardare, e sono il motivo per cui i semafori battono l'attesa
// attiva della lock variable.
//
// La prima: S può andare NEGATIVO, e quando è negativo il suo valore assoluto
// È il numero di processi in coda. Non è un effetto collaterale, è
// informazione: S = -2 significa "risorsa occupata e due in attesa".
//
// La seconda: un processo bloccato non gira a vuoto. Esce dalla coda dei pronti
// e non consuma CPU finché non viene svegliato.
//
// Con S inizializzato a 1 il semaforo è un MUTEX e garantisce mutua esclusione.
// Con S inizializzato a n gestisce n copie di una risorsa. E usato male — due
// semafori presi in ordine opposto da due processi — produce un DEADLOCK.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/threads.hpp"

static Tracer T("semaphores", "Semafori", "Sincronizzazione", __FILE__);

using trace::ThreadProc;

struct Semaphore {
    std::string name;
    int value;
    std::vector<int> queue;
};

enum OpKind { WAIT, SIGNAL, ENTER, USE, EXIT, END };

struct Op {
    OpKind kind;
    int sem;
};

static std::vector<Semaphore> sems;
static std::vector<std::vector<Op>> programs;
static std::vector<ThreadProc> procs;
static int blockedCount = 0;

// tracer-hide-begin
static std::string lineFor(const Op& op) {
    switch (op.kind) {
        case WAIT:   return "wait(" + sems[op.sem].name + ");";
        case SIGNAL: return "signal(" + sems[op.sem].name + ");";
        case ENTER:  return "// ---- risorsa acquisita ----";
        case USE:    return "usa la risorsa;";
        case EXIT:   return "// ---- risorsa rilasciata ----";
        default:     return "// terminato";
    }
}

static trace::Shared sharedVars() {
    trace::Shared out;
    for (const Semaphore& s : sems) out.push_back({s.name, std::to_string(s.value)});
    return out;
}

static std::vector<std::string> blockedNames() {
    std::vector<std::string> out;
    for (const Semaphore& s : sems)
        for (int i : s.queue) out.push_back(procs[i].name + " (su " + s.name + ")");
    return out;
}
// tracer-hide-end

std::string stepProcess(int i) {
    ThreadProc& p = procs[i];
    const Op& op = programs[i][p.pc];

    switch (op.kind) {
        case WAIT: {
            Semaphore& S = sems[op.sem];
            S.value--;
            if (S.value < 0) {
                p.blocked = true;
                S.queue.push_back(i);
                blockedCount++;
                return p.name + " esegue wait(" + S.name + "): " + S.name + " scende a " +
                       std::to_string(S.value) + ". Essendo negativo, la risorsa non è disponibile: " +
                       p.name + " si BLOCCA e entra in coda. |" + std::to_string(S.value) + "| = " +
                       std::to_string(-S.value) + " è esattamente il numero di processi in attesa su " +
                       S.name + ".";
            }
            p.pc++;
            return p.name + " esegue wait(" + S.name + "): " + S.name + " scende a " +
                   std::to_string(S.value) + ", non negativo, quindi c'era una copia libera e " +
                   p.name + " prosegue senza bloccarsi.";
        }
        case SIGNAL: {
            Semaphore& S = sems[op.sem];
            S.value++;
            std::string extra;
            if (S.value <= 0 && !S.queue.empty()) {
                int woken = S.queue.front();
                S.queue.erase(S.queue.begin());
                procs[woken].blocked = false;
                procs[woken].pc++;
                extra = " " + S.name + " è ancora <= 0, quindi qualcuno stava aspettando: " +
                        procs[woken].name + " viene SVEGLIATO, esce dalla coda e riprende dalla riga "
                        "dopo la sua wait.";
            }
            p.pc++;
            return p.name + " esegue signal(" + S.name + "): " + S.name + " sale a " +
                   std::to_string(S.value) + "." + extra;
        }
        case ENTER:
            p.critical = true;
            p.pc++;
            return p.name + " ha la risorsa in mano.";
        case USE:
            p.pc++;
            return p.name + " usa la risorsa.";
        case EXIT:
            p.critical = false;
            p.pc++;
            return p.name + " ha finito di usare la risorsa; ora deve restituirla con signal.";
        default:
            p.done = true;
            return p.name + " ha terminato.";
    }
}

// Programma standard: prendi, usa, restituisci.
static std::vector<Op> userProgram(int sem) {
    return {{WAIT, sem}, {ENTER, sem}, {USE, sem}, {EXIT, sem}, {SIGNAL, sem}, {END, 0}};
}

static void run(const char* label, const std::vector<Semaphore>& semaphores,
                const std::vector<std::vector<Op>>& progs, const std::vector<int>& schedule,
                int maxInCritical, const char* intro) {
    sems = semaphores;
    programs = progs;
    blockedCount = 0;

    procs.clear();
    for (size_t i = 0; i < progs.size(); i++) {
        ThreadProc p;
        p.name = "P" + std::to_string(i);
        for (const Op& op : progs[i]) p.lines.push_back(lineFor(op));
        procs.push_back(p);
    }

    T.input(label);
    T.at(__LINE__, "main")
     .threads(trace::threadsJson(procs, -1, sharedVars(), {}, true, maxInCritical))
     .counters({{"bloccati", 0}})
     .note(intro)
     .emit();

    size_t k = 0;
    for (int guard = 0; guard < 90; guard++) {
        bool allDone = true;
        bool anyRunnable = false;
        for (const ThreadProc& p : procs) {
            if (!p.done) allDone = false;
            if (!p.done && !p.blocked) anyRunnable = true;
        }
        if (allDone) break;

        // Nessuno può avanzare e nessuno ha finito: ognuno aspetta una risorsa
        // che è in mano a un altro che a sua volta aspetta. Nessun signal
        // arriverà mai, perché chi dovrebbe emetterlo è bloccato.
        if (!anyRunnable) {
            int stuck = 0;
            for (const ThreadProc& p : procs) if (p.blocked) stuck++;
            T.at(__LINE__, "main")
             .vars({{"processi bloccati", stuck}})
             .threads(trace::threadsJson(procs, -1, sharedVars(), blockedNames(), false, maxInCritical))
             .error("deadlock")
             .counters({{"bloccati", stuck}})
             .note("DEADLOCK. Tutti i processi non terminati sono bloccati, e ognuno aspetta un "
                   "semaforo che è in mano a un altro processo bloccato. Nessuno potrà mai eseguire "
                   "la signal che sbloccherebbe gli altri: il sistema è fermo per sempre. Non è un "
                   "bug dei semafori, è un bug dell'ORDINE in cui vengono presi.")
             .emit();
            return;
        }

        int who = schedule[k % schedule.size()];
        k++;
        if (procs[who].done || procs[who].blocked) continue;

        std::string note = stepProcess(who);
        int inside = 0;
        for (const ThreadProc& p : procs) if (p.critical) inside++;
        int stuck = 0;
        for (const ThreadProc& p : procs) if (p.blocked) stuck++;

        T.at(__LINE__, "main")
         .vars({{"in esecuzione", procs[who].name}, {"dentro", inside}, {"bloccati", stuck}})
         .threads(trace::threadsJson(procs, who, sharedVars(), blockedNames(), false, maxInCritical))
         .activeThread(procs[who].name)
         .counters({{"bloccati", stuck}, {"dentro", inside}})
         .note(inside > maxInCritical
                 ? note + "  ⚠ Sono dentro in " + std::to_string(inside) + ", il massimo era " +
                   std::to_string(maxInCritical) + "."
                 : note)
         .emit();
    }

    T.at(__LINE__, "main")
     .threads(trace::threadsJson(procs, -1, sharedVars(), blockedNames(), false, maxInCritical))
     .counters({{"bloccati", 0}})
     .note("Tutti i processi hanno terminato e i semafori sono tornati al valore iniziale: nessuna "
           "risorsa è rimasta occupata.")
     .emit();
}

int main() {
    T.view("threads").complexity("O(1) per wait/signal", "O(n) per la coda");

    // 1. wait / signal sul valore: tre processi, una sola risorsa.
    run("wait / signal: il valore va sotto zero",
        {{"S", 1, {}}},
        {userProgram(0), userProgram(0), userProgram(0)},
        {0, 1, 2}, 1,
        "Tre processi, una sola copia della risorsa (S = 1). Guarda S scendere sotto zero e la coda "
        "dei bloccati riempirsi: il valore negativo conta esattamente chi aspetta.");

    // 2. Semaforo binario usato come mutex.
    run("semaforo binario (mutex)",
        {{"mutex", 1, {}}},
        {userProgram(0), userProgram(0)},
        {0, 1}, 1,
        "S inizializzato a 1: è un mutex. Con lo stesso interleaving alternato che rompeva la lock "
        "variable, qui il secondo processo si blocca invece di entrare. La differenza è che wait è "
        "atomica: il test e il decremento non si possono separare.");

    // 3. Semaforo contatore: i 5 stampanti dell'esempio del libro.
    run("semaforo contatore: 5 stampanti, 6 processi",
        {{"stampanti", 5, {}}},
        {userProgram(0), userProgram(0), userProgram(0),
         userProgram(0), userProgram(0), userProgram(0)},
        {0, 1, 2, 3, 4, 5}, 5,
        "Cinque stampanti e sei processi che le vogliono. I primi cinque entrano tutti insieme — e va "
        "benissimo, non è una violazione — il sesto trova S = -1 e aspetta.");

    // 4. Deadlock: due semafori presi in ordine opposto.
    run("deadlock: due semafori in ordine opposto",
        {{"S1", 1, {}}, {"S2", 1, {}}},
        {{{WAIT, 0}, {WAIT, 1}, {ENTER, 0}, {USE, 0}, {EXIT, 0}, {SIGNAL, 1}, {SIGNAL, 0}, {END, 0}},
         {{WAIT, 1}, {WAIT, 0}, {ENTER, 1}, {USE, 1}, {EXIT, 1}, {SIGNAL, 0}, {SIGNAL, 1}, {END, 0}}},
        {0, 1}, 1,
        "P0 prende S1 poi S2; P1 prende S2 poi S1. Ordini opposti. Bastano due passi a testa perché "
        "ognuno tenga ciò che serve all'altro.");

    T.dump("traces/semaphores.js");
    return 0;
}
