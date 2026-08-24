// Peterson's Solution
//
// Correct mutual exclusion for two processes, with no hardware support at
// all: just two shared variables and three lines of protocol.
//
//     flag[i] = true;                  // I declare I want in
//     turn = j;                        // yield the turn to the other
//     while (flag[j] && turn == j);    // wait only if it's really needed
//     ... critical section ...
//     flag[i] = false;
//
// The line that looks absurd is the second: why yield the turn right when
// you want to get in? That's where everything lives. If both declare their
// intent, both yield, but turn is ONE variable: the second write erases the
// first, so turn ends up pointing at only one of the two. Whoever wrote
// last loses, the other gets in. There's no way the waiting condition is
// false for both at the same moment.
//
// The same interleaving that breaks the lock variable holds up here.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/threads.hpp"

static Tracer T("petersons_solution", "Peterson's Solution", "Synchronization", __FILE__);

using trace::ThreadProc;

static const std::vector<std::string> PROGRAM = {
    "flag[i] = true;                  // I want in",
    "turn = j;                        // yield the turn to the other",
    "while (flag[j] && turn == j);    // wait",
    "// ---- critical section begins ----",
    "tmp = counter;",
    "counter = tmp + 1;",
    "flag[i] = false;                 // exit",
    "// done",
};

static std::vector<ThreadProc> procs;
static bool flagArr[2] = {false, false};
static int turn = 0;
static int counter = 0;
static int tmp[2] = {0, 0};
static int waits = 0;

// tracer-hide-begin
static trace::Shared sharedVars() {
    return {{"flag[0]", flagArr[0] ? "true" : "false"},
            {"flag[1]", flagArr[1] ? "true" : "false"},
            {"turn", std::to_string(turn)},
            {"counter", std::to_string(counter)}};
}
// tracer-hide-end

std::string stepProcess(int i) {
    ThreadProc& p = procs[i];
    const int j = 1 - i;

    switch (p.pc) {
        case 0:
            flagArr[i] = true;
            p.pc = 1;
            return p.name + " raises its own flag: declares it wants in. On its own this line isn't "
                            "enough, since the other one can raise it too.";
        case 1:
            turn = j;
            p.pc = 2;
            return p.name + " writes turn = " + std::to_string(j) +
                   ", i.e. yields the turn to the other. Looks counterproductive, but it's the move "
                   "that breaks the symmetry: if both write, the last write wins and only one is left "
                   "\"pointed at\" by turn.";
        case 2:
            if (flagArr[j] && turn == j) {
                waits++;
                return p.name + " waits: the other wants in (flag[" + std::to_string(j) +
                       "] = true) AND it's their turn (turn = " + std::to_string(turn) +
                       "). Both conditions are true, so it yields.";
            }
            p.pc = 3;
            return p.name + " proceeds: " +
                   (flagArr[j] ? "the other wants in, but turn = " + std::to_string(turn) +
                                 " points to " + procs[turn].name + ", so it's my turn."
                               : "the other hasn't raised its flag, the way is clear.");
        case 3:
            p.critical = true;
            p.pc = 4;
            return p.name + " enters the critical section. The other, if it tries, will stay on the "
                            "waiting line: the two conditions can't be false for both at once.";
        case 4:
            tmp[i] = counter;
            p.pc = 5;
            return p.name + " reads counter (" + std::to_string(counter) + ") into tmp.";
        case 5:
            counter = tmp[i] + 1;
            p.pc = 6;
            return p.name + " writes counter = " + std::to_string(counter) +
                   ". No lost update: it was the only one working on it.";
        case 6:
            flagArr[i] = false;
            p.critical = false;
            p.pc = 7;
            return p.name + " lowers its flag and exits. Now the other can stop waiting.";
        default:
            p.done = true;
            return p.name + " has finished.";
    }
}

static void run(const char* label, const std::vector<int>& schedule, const char* intro) {
    procs = {{"P0", PROGRAM, 0, false, false, false}, {"P1", PROGRAM, 0, false, false, false}};
    flagArr[0] = flagArr[1] = false;
    turn = 0;
    counter = 0;
    tmp[0] = tmp[1] = 0;
    waits = 0;

    T.input(label);
    T.at(__LINE__, "main")
     .threads(trace::threadsJson(procs, -1, sharedVars(), {}, true))
     .counters({{"counter", counter}, {"waits", 0}})
     .note(intro)
     .emit();

    size_t k = 0;
    for (int guard = 0; guard < 80; guard++) {
        if (procs[0].done && procs[1].done) break;
        int who = schedule[k % schedule.size()];
        k++;
        if (procs[who].done) continue;

        std::string note = stepProcess(who);
        int inCritical = 0;
        for (const ThreadProc& p : procs) if (p.critical) inCritical++;

        T.at(__LINE__, "main")
         .vars({{"running", procs[who].name}, {"turn", turn},
                {"flag[0]", flagArr[0]}, {"flag[1]", flagArr[1]}})
         .threads(trace::threadsJson(procs, who, sharedVars(), {}, false))
         .activeThread(procs[who].name)
         .counters({{"counter", counter}, {"waits", waits}})
         .note(inCritical > 1 ? note + "  ⚠ VIOLATION — this shouldn't be able to happen." : note)
         .emit();
    }

    T.at(__LINE__, "main")
     .vars({{"counter", counter}})
     .threads(trace::threadsJson(procs, -1, sharedVars(), {}, false))
     .counters({{"counter", counter}, {"waits", waits}})
     .note("Done, counter = " + std::to_string(counter) +
           ". Never two processes together in the critical section, with any of these interleavings — "
           "including the one that was fatal for the lock variable.")
     .emit();
}

int main() {
    T.view("threads").complexity("—", "O(1)");

    run("the interleaving that broke the lock variable", {0, 1},
        "Strict alternation every step: it's exactly the scenario that let two processes in "
        "together with the lock variable. Here it doesn't happen.");
    run("both declare, then contend for the turn", {0, 0, 1, 1, 0, 1, 0, 1},
        "Both raise their flag before either reaches the while: the worst case for a "
        "mutual-exclusion protocol.");
    run("P0 gets a head start", {0, 0, 0, 0, 1, 1, 1, 0, 0, 1},
        "P0 reaches the critical section before P1 raises its flag: P1 then waits.");

    T.dump("traces/petersons_solution.js");
    return 0;
}
