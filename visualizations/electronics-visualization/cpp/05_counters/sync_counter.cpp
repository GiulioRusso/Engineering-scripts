// Synchronous Counter — The Flip-Flop Chain, Step by Step
//
// Same clock for every stage: no ripple, every bit changes at the same
// instant. What varies from stage to stage is only the toggle signal: bit0
// always toggles, bit1 toggles only when bit0 is 1, bit2 only when bit0 AND
// bit1 are both 1 — the chain of ANDs generating these enables is the only
// difference from a ripple counter.
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/circuit.hpp"

static Tracer T("sync_counter", "Synchronous Counter", "Counters", __FILE__);
static const int NBITS = 4;

static Schematic layout() {
    Schematic S;
    for (int i = 0; i < NBITS; i++) S.gate("ff" + std::to_string(i), "FF", 1.0 + i * 2.2, 1.2);
    S.pin("CLK", 0.0, 1.2, "left");
    for (int i = 0; i < NBITS; i++) S.wire("CLK", "ff" + std::to_string(i) + ".a");
    return S;
}

int main() {
    T.view("schematic").complexity("O(1) per stage", "4 T-FF + AND");
    T.panel("array", "Counter bits");

    Schematic sch = layout();
    std::vector<int> Q(NBITS, 0);
    T.clearWatches();
    T.watchVector("bit", Q);

    T.input("0 .. 15 and wrap-around");
    for (int cycle = 0; cycle < 18; cycle++) {
        int T0 = 1;
        int T1 = Q[0];
        int T2 = Q[0] & Q[1];
        int T3 = Q[0] & Q[1] & Q[2];
        int toggles[NBITS] = {T0, T1, T2, T3};
        std::map<std::string, int> v{{"CLK", 1}};
        for (int i = 0; i < NBITS; i++) {
            if (toggles[i]) Q[i] = 1 - Q[i];
            v["ff" + std::to_string(i)] = Q[i];
        }
        int count = Q[3] * 8 + Q[2] * 4 + Q[1] * 2 + Q[0];
        T.at(__LINE__, __func__)
         .vars({{"count", count}, {"T0", T0}, {"T1", T1}, {"T2", T2}, {"T3", T3}})
         .circuitState(sch.json(v))
         .note("Rising edge: T0=1 always, T1=" + std::to_string(T1) + " (=bit0), T2=" +
               std::to_string(T2) + " (=bit0 AND bit1), T3=" + std::to_string(T3) +
               ". New count = " + std::to_string(count) + ".")
         .emit();
    }

    T.dump("traces/sync_counter.js");
}
