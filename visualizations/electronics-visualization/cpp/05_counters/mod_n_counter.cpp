// Mod-N Counter — Reset Before the Natural Wrap
//
// A 4-bit counter would naturally count up to 15. To stop earlier (mod 6,
// mod 10, ...) it needs to detect the target count and force a synchronous
// reset, one cycle before the natural ripple would have done it on its own.
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/circuit.hpp"
#include "../sim/waves.hpp"

static Tracer T("mod_n_counter", "Mod-N Counter", "Counters", __FILE__);
static const int NBITS = 4;
static const int CYCLES = 24;

static Schematic layout() {
    Schematic S;
    for (int i = 0; i < NBITS; i++) S.gate("ff" + std::to_string(i), "FF", 1.0 + i * 2.2, 1.2);
    S.gate("dec", "AND", 1.0 + NBITS * 2.2, 1.2);   // reset decoder: count == N-1 ?
    S.pin("CLK", 0.0, 1.2, "left");
    for (int i = 0; i < NBITS; i++) S.wire("CLK", "ff" + std::to_string(i) + ".a");
    return S;
}

void run(const std::string& label, int N) {
    Schematic sch = layout();
    std::vector<int> Q(NBITS, 0);
    std::vector<int> countHist(CYCLES, 0), resetHist(CYCLES, 0), clkHist(CYCLES, 1);

    T.input(label);
    for (int cycle = 0; cycle < CYCLES; cycle++) {
        int T0 = 1, T1 = Q[0], T2 = Q[0] & Q[1], T3 = Q[0] & Q[1] & Q[2];
        int toggles[NBITS] = {T0, T1, T2, T3};
        for (int i = 0; i < NBITS; i++) if (toggles[i]) Q[i] = 1 - Q[i];
        int natural = Q[3] * 8 + Q[2] * 4 + Q[1] * 2 + Q[0];
        bool reset = natural == N;
        if (reset) for (int i = 0; i < NBITS; i++) Q[i] = 0;
        int count = Q[3] * 8 + Q[2] * 4 + Q[1] * 2 + Q[0];

        countHist[cycle] = count;
        resetHist[cycle] = reset ? 1 : 0;

        std::map<std::string, int> v{{"CLK", 1}};
        for (int i = 0; i < NBITS; i++) v["ff" + std::to_string(i)] = Q[i];
        v["dec"] = reset ? 1 : 0;

        Waves W;
        W.add("CLK", "clk", clkHist);
        W.add("reset", "data", resetHist);
        W.add("count", "out", countHist);
        T.at(__LINE__, __func__)
         .vars({{"count", count}, {"target", N}})
         .circuitState(sch.json(v, reset ? "dec" : ""))
         .waveState(W.json(cycle))
         .note(reset ? "Decoder detects count == " + std::to_string(N) +
                       ": synchronous reset forced to 0."
                     : "count = " + std::to_string(count) + ".")
         .emit();
    }
}

int main() {
    T.view("schematic").complexity("O(1) per stage", "4 T-FF + decoder");
    T.panel("waveform", "Count and reset");

    run("Mod 6", 6);
    run("Mod 10", 10);

    T.dump("traces/mod_n_counter.js");
}
