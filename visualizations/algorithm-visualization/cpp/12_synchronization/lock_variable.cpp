// Lock variable
//
// The most naive attempt at mutual exclusion: a shared lock variable.
// Whoever wants in waits for it to be 0, then sets it to 1.
//
//     while (lock != 0);   // wait
//     lock = 1;            // take the lock
//     ... critical section ...
//     lock = 0;             // release
//
// And it doesn't work. Between the test and the assignment there's an
// instant where the process can be suspended, and in that instant another
// process can do the same test and still find 0. Both get in.
//
// The flaw isn't in the logic but in ATOMICITY: "read then write" are two
// operations, and anything can happen in between. It needs a hardware
// instruction that does both together (test-and-set), or a software
// protocol like Peterson's.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/threads.hpp"

static Tracer T("lock_variable", "Lock variable", "Synchronization", __FILE__);

using trace::ThreadProc;

// Each process's "program". One line = one indivisible step: that
// granularity is exactly the point of the lesson.
static const std::vector<std::string> PROGRAM = {
    "while (lock != 0);            // busy wait",
    "lock = 1;                     // take the lock",
    "// ---- critical section begins ----",
    "tmp = counter;",
    "counter = tmp + 1;",
    "lock = 0;                     // release the lock",
    "// done",
};

static std::vector<ThreadProc> procs;
static int lockVar = 0;
static int counter = 0;
static int tmp[2] = {0, 0};
static int busyWaits = 0;

// tracer-hide-begin
static trace::Shared sharedVars() {
    return {{"lock", std::to_string(lockVar)},
            {"counter", std::to_string(counter)},
            {"tmp[P0]", std::to_string(tmp[0])},
            {"tmp[P1]", std::to_string(tmp[1])}};
}
static bool firstStep = true;
// tracer-hide-end

// Runs ONE step of process i and returns the note describing what just
// happened. No line does more than one thing.
std::string stepProcess(int i) {
    ThreadProc& p = procs[i];

    switch (p.pc) {
        case 0:
            if (lockVar != 0) {
                busyWaits++;
                return p.name + " tests lock: it's " + std::to_string(lockVar) +
                       ", so it stays stuck on this line and tries again. It doesn't sleep: it burns "
                       "a CPU turn doing nothing. That's BUSY WAITING, and on a single core it's pure waste.";
            }
            p.pc = 1;
            return p.name + " tests lock and finds it 0: the way looks clear, moves to the "
                            "next line. WARNING: there's a gap between this test and the assignment, "
                            "and the process can be suspended right here.";
        case 1:
            lockVar = 1;
            p.pc = 2;
            return p.name + " writes lock = 1. If someone else had already passed the "
                            "test in the meantime, this write arrives too late to stop them.";
        case 2:
            p.critical = true;
            p.pc = 3;
            return p.name + " enters the critical section.";
        case 3:
            tmp[i] = counter;
            p.pc = 4;
            return p.name + " reads counter (" + std::to_string(counter) + ") into its "
                            "local variable tmp.";
        case 4:
            counter = tmp[i] + 1;
            p.pc = 5;
            return p.name + " writes counter = tmp + 1 = " + std::to_string(counter) + ".";
        case 5:
            lockVar = 0;
            p.critical = false;
            p.pc = 6;
            return p.name + " releases the lock and exits the critical section.";
        default:
            p.done = true;
            return p.name + " has finished.";
    }
}

static void run(const char* label, const std::vector<int>& schedule, const char* intro) {
    procs = {{"P0", PROGRAM, 0, false, false, false}, {"P1", PROGRAM, 0, false, false, false}};
    lockVar = 0;
    counter = 0;
    tmp[0] = tmp[1] = 0;
    busyWaits = 0;
    firstStep = true;

    T.input(label);
    T.at(__LINE__, "main")
     .threads(trace::threadsJson(procs, -1, sharedVars(), {}, true))
     .counters({{"counter", counter}, {"busy waits", 0}})
     .note(intro)
     .emit();
    firstStep = false;

    // The scheduler is explicit: this list says whose turn it is at each
    // step. There's no randomness, and the interleaving can be chosen.
    size_t k = 0;
    for (int guard = 0; guard < 60; guard++) {
        if (procs[0].done && procs[1].done) break;
        int who = schedule[k % schedule.size()];
        k++;
        if (procs[who].done) continue;

        std::string note = stepProcess(who);
        int violating = 0;
        for (const ThreadProc& p : procs) if (p.critical) violating++;

        T.at(__LINE__, "main")
         .vars({{"running", procs[who].name}, {"lock", lockVar}, {"counter", counter}})
         .threads(trace::threadsJson(procs, who, sharedVars(), {}, false))
         .activeThread(procs[who].name)
         .counters({{"counter", counter}, {"busy waits", busyWaits}})
         .note(violating > 1
                 ? note + "  ⚠ NOW THERE ARE TWO IN THE CRITICAL SECTION: both lines are "
                          "highlighted. Mutual exclusion has failed."
                 : note)
         .emit();
    }

    T.at(__LINE__, "main")
     .vars({{"counter", counter}, {"expected", 2}})
     .threads(trace::threadsJson(procs, -1, sharedVars(), {}, false))
     .counters({{"counter", counter}, {"busy waits", busyWaits}})
     .note(counter == 2
             ? "Both finished and counter is 2, as it should be. With this interleaving the lock "
               "held — but \"held this time\" is not a guarantee."
             : "Both finished and counter is " + std::to_string(counter) +
               " instead of 2. There were two increments, but one was lost: they had read the same "
               "starting value. That's the LOST UPDATE, the practical consequence of the "
               "violation.")
     .emit();
}

int main() {
    T.view("threads").complexity("—", "O(1)");

    // Strict alternation from the very first step: both pass the test before
    // either one gets to write lock = 1.
    run("interleaving THAT BREAKS mutual exclusion", {0, 1},
        "Two processes, the same code, one lock. The scheduler alternates every step. "
        "Watch the first two steps: they're the ones that break everything.");

    // P0 manages to take the lock before P1 tests it.
    run("interleaving that works (and shows busy waiting)", {0, 0, 1, 1, 1, 0, 0, 0, 0, 1},
        "Same code, different interleaving: P0 manages to write lock = 1 before P1 reaches the test. "
        "P1 spins idle on the first line until the lock frees up.");

    T.dump("traces/lock_variable.js");
    return 0;
}
