// Semaphores
//
// A semaphore is an integer with only two operations, both atomic:
//
//     wait(S):    S--;  if S < 0  →  the process BLOCKS and joins the queue
//     signal(S):  S++;  if S <= 0 →  wakes one process from the queue
//
// Two things to watch, and they're why semaphores beat the lock variable's
// busy waiting.
//
// The first: S can go NEGATIVE, and when it's negative its absolute value
// IS the number of processes in the queue. It's not a side effect, it's
// information: S = -2 means "resource busy and two waiting".
//
// The second: a blocked process doesn't spin idle. It leaves the ready
// queue and burns no CPU until it's woken up.
//
// With S initialized to 1 the semaphore is a MUTEX and guarantees mutual
// exclusion. With S initialized to n it manages n copies of a resource. And
// used wrong — two semaphores taken in opposite order by two processes —
// it produces a DEADLOCK.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/threads.hpp"

static Tracer T("semaphores", "Semaphores", "Synchronization", __FILE__);

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
        case ENTER:  return "// ---- resource acquired ----";
        case USE:    return "use the resource;";
        case EXIT:   return "// ---- resource released ----";
        default:     return "// done";
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
        for (int i : s.queue) out.push_back(procs[i].name + " (on " + s.name + ")");
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
                return p.name + " runs wait(" + S.name + "): " + S.name + " drops to " +
                       std::to_string(S.value) + ". Being negative, the resource isn't available: " +
                       p.name + " BLOCKS and joins the queue. |" + std::to_string(S.value) + "| = " +
                       std::to_string(-S.value) + " is exactly the number of processes waiting on " +
                       S.name + ".";
            }
            p.pc++;
            return p.name + " runs wait(" + S.name + "): " + S.name + " drops to " +
                   std::to_string(S.value) + ", not negative, so there was a free copy and " +
                   p.name + " continues without blocking.";
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
                extra = " " + S.name + " is still <= 0, so someone was waiting: " +
                        procs[woken].name + " gets WOKEN UP, leaves the queue and resumes at the line "
                        "after its wait.";
            }
            p.pc++;
            return p.name + " runs signal(" + S.name + "): " + S.name + " rises to " +
                   std::to_string(S.value) + "." + extra;
        }
        case ENTER:
            p.critical = true;
            p.pc++;
            return p.name + " has the resource in hand.";
        case USE:
            p.pc++;
            return p.name + " uses the resource.";
        case EXIT:
            p.critical = false;
            p.pc++;
            return p.name + " is done using the resource; now it must give it back with signal.";
        default:
            p.done = true;
            return p.name + " has finished.";
    }
}

// Standard program: acquire, use, give back.
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
     .counters({{"blocked", 0}})
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

        // Nobody can advance and nobody is done: everyone is waiting on a
        // resource held by someone else who is in turn waiting. No signal
        // will ever arrive, because whoever should send it is blocked.
        if (!anyRunnable) {
            int stuck = 0;
            for (const ThreadProc& p : procs) if (p.blocked) stuck++;
            T.at(__LINE__, "main")
             .vars({{"blocked processes", stuck}})
             .threads(trace::threadsJson(procs, -1, sharedVars(), blockedNames(), false, maxInCritical))
             .error("deadlock")
             .counters({{"blocked", stuck}})
             .note("DEADLOCK. Every unfinished process is blocked, and each is waiting on a "
                   "semaphore held by another blocked process. Nobody will ever be able to run the "
                   "signal that would free the others: the system is stuck forever. It's not a bug "
                   "in semaphores, it's a bug in the ORDER they're acquired in.")
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
         .vars({{"running", procs[who].name}, {"inside", inside}, {"blocked", stuck}})
         .threads(trace::threadsJson(procs, who, sharedVars(), blockedNames(), false, maxInCritical))
         .activeThread(procs[who].name)
         .counters({{"blocked", stuck}, {"inside", inside}})
         .note(inside > maxInCritical
                 ? note + "  ⚠ There are " + std::to_string(inside) + " inside, the max was " +
                   std::to_string(maxInCritical) + "."
                 : note)
         .emit();
    }

    T.at(__LINE__, "main")
     .threads(trace::threadsJson(procs, -1, sharedVars(), blockedNames(), false, maxInCritical))
     .counters({{"blocked", 0}})
     .note("All processes have finished and the semaphores are back to their initial value: no "
           "resource is left held.")
     .emit();
}

int main() {
    T.view("threads").complexity("O(1) per wait/signal", "O(n) for the queue");

    // 1. wait / signal on the value: three processes, one resource.
    run("wait / signal: the value goes below zero",
        {{"S", 1, {}}},
        {userProgram(0), userProgram(0), userProgram(0)},
        {0, 1, 2}, 1,
        "Three processes, a single copy of the resource (S = 1). Watch S drop below zero and the "
        "blocked queue fill up: the negative value counts exactly who's waiting.");

    // 2. Binary semaphore used as a mutex.
    run("binary semaphore (mutex)",
        {{"mutex", 1, {}}},
        {userProgram(0), userProgram(0)},
        {0, 1}, 1,
        "S initialized to 1: it's a mutex. With the same alternating interleaving that broke the lock "
        "variable, here the second process blocks instead of getting in. The difference is that wait is "
        "atomic: the test and the decrement can't be pulled apart.");

    // 3. Counting semaphore: the book example's 5 printers.
    run("counting semaphore: 5 printers, 6 processes",
        {{"printers", 5, {}}},
        {userProgram(0), userProgram(0), userProgram(0),
         userProgram(0), userProgram(0), userProgram(0)},
        {0, 1, 2, 3, 4, 5}, 5,
        "Five printers and six processes that want them. The first five all get in together — and "
        "that's fine, not a violation — the sixth finds S = -1 and waits.");

    // 4. Deadlock: two semaphores taken in opposite order.
    run("deadlock: two semaphores in opposite order",
        {{"S1", 1, {}}, {"S2", 1, {}}},
        {{{WAIT, 0}, {WAIT, 1}, {ENTER, 0}, {USE, 0}, {EXIT, 0}, {SIGNAL, 1}, {SIGNAL, 0}, {END, 0}},
         {{WAIT, 1}, {WAIT, 0}, {ENTER, 1}, {USE, 1}, {EXIT, 1}, {SIGNAL, 0}, {SIGNAL, 1}, {END, 0}}},
        {0, 1}, 1,
        "P0 takes S1 then S2; P1 takes S2 then S1. Opposite orders. Two steps each is enough for "
        "each to be holding what the other needs.");

    T.dump("traces/semaphores.js");
    return 0;
}
