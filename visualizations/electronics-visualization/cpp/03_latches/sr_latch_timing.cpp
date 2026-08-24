// SR Latch Over Time — Set, Hold, Reset
//
// The same cross-coupled NOR pair from the schematic view, now watched as a
// waveform: a pulse on S raises Q and holds it even after S returns to 0 —
// that's memory. The gate-by-gate settling detail stays in the "SR Latch
// (NOR)" example; here the interest is the behaviour over time.
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/logic.hpp"
#include "../sim/circuit.hpp"
#include "../sim/waves.hpp"

static Tracer T("sr_latch_timing", "SR Over Time", "Latches", __FILE__);
static const int N = 20;

static Schematic layout() {
    Schematic S;
    S.gate("g1", "NOR", 2.2, 0.8);
    S.gate("g2", "NOR", 2.2, 2.2);
    S.pin("R", 0.0, 0.8, "left");
    S.pin("S", 0.0, 2.2, "left");
    S.wire("R", "g1.a");
    S.wire("g2", "g1.b");
    S.wire("S", "g2.a");
    S.wire("g1", "g2.b");
    S.pin("Q", 4.0, 0.8, "right");
    S.pin("Qb", 4.0, 2.2, "right");
    S.wire("g1", "Q");
    S.wire("g2", "Qb");
    return S;
}

// No tracer calls: only the per-t waveform steps below are traced, so the
// settle order doesn't matter here the way it does in sr_latch_nor.cpp.
static void settleQuiet(int S, int R, int& Q, int& Qb) {
    for (int pass = 0; pass < 6; pass++) {
        int newQ = NOR(R, Qb), newQb = NOR(S, Q);
        bool changed = (newQ != Q) || (newQb != Qb);
        Q = newQ;
        Qb = newQb;
        if (!changed) break;
    }
}

int main() {
    T.view("waveform").complexity("level-sensitive", "2 NOR");
    T.panel("schematic", "Schematic");

    std::vector<int> Sv(N, 0), Rv(N, 0), Qv(N), Qbv(N);
    for (int t : {3, 4}) Sv[t] = 1;
    for (int t : {11, 12}) Rv[t] = 1;

    int Q = 0, Qb = 1;
    for (int t = 0; t < N; t++) {
        settleQuiet(Sv[t], Rv[t], Q, Qb);
        Qv[t] = Q;
        Qbv[t] = Qb;
    }

    Waves W;
    W.add("S", "data", Sv);
    W.add("R", "data", Rv);
    W.add("Q", "out", Qv);
    W.add("Qb", "out", Qbv);
    Schematic sch = layout();

    T.input("Pulse on S then pulse on R");
    for (int t = 0; t < N; t++) {
        std::map<std::string, int> v{{"S", Sv[t]}, {"R", Rv[t]}, {"g1", Qv[t]}, {"g2", Qbv[t]},
                                      {"Q", Qv[t]}, {"Qb", Qbv[t]}};
        T.at(__LINE__, __func__)
         .vars({{"t", t}, {"S", Sv[t]}, {"R", Rv[t]}, {"Q", Qv[t]}, {"Qb", Qbv[t]}})
         .waveState(W.json(t))
         .circuitState(sch.json(v))
         .note(Sv[t] ? "S=1: set, Q rises to 1."
               : Rv[t] ? "R=1: reset, Q falls to 0."
               : "S=R=0: hold, Q stays " + std::to_string(Qv[t]) + " thanks to the cross-coupled feedback.")
         .emit();
    }

    T.dump("traces/sr_latch_timing.js");
}
