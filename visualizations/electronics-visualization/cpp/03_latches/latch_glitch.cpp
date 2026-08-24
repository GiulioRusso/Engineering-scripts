// The Glitch That Survives — Why Transparency Is Dangerous
//
// A very brief pulse on D, entirely inside the window where EN=1, gets
// copied onto Q just like any other value — the latch has no way to tell a
// "real" data change from a one-instant disturbance apart. If the same pulse
// arrived while EN was already low, Q would never see it: the difference is
// all in the timing, not in the latch itself.
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/logic.hpp"
#include "../sim/waves.hpp"

static Tracer T("latch_glitch", "The Glitch That Survives", "Latches", __FILE__);
static const int N = 20;

static void settleQuiet(int D, int EN, int& Q, int& Qb) {
    int Sb = NAND(D, EN), Rb = NAND(Sb, EN);
    for (int pass = 0; pass < 6; pass++) {
        int newQ = NAND(Sb, Qb), newQb = NAND(Rb, Q);
        bool changed = (newQ != Q) || (newQb != Qb);
        Q = newQ;
        Qb = newQb;
        if (!changed) break;
    }
}

void run(const std::string& label, int glitchPos, int enFall) {
    std::vector<int> D(N, 0), EN(N, 0), Qv(N);
    for (int t = 2; t < enFall; t++) EN[t] = 1;
    D[glitchPos] = 1;

    int Q = 0, Qb = 1;
    for (int t = 0; t < N; t++) {
        settleQuiet(D[t], EN[t], Q, Qb);
        Qv[t] = Q;
    }

    Waves W;
    W.add("D", "data", D);
    W.add("EN", "data", EN);
    W.add("Q", "out", Qv);

    T.input(label);
    for (int t = 0; t < N; t++) {
        T.at(__LINE__, __func__)
         .vars({{"t", t}, {"D", D[t]}, {"EN", EN[t]}, {"Q", Qv[t]}})
         .waveState(W.json(t))
         .note(t == glitchPos
               ? (EN[t] ? "The glitch on D arrives while EN=1: the latch is transparent and copies it onto Q."
                        : "The glitch on D arrives at EN=0: the latch is latched, it never sees it.")
               : "Q = " + std::to_string(Qv[t]) + ".")
         .emit();
    }
}

int main() {
    T.view("waveform").complexity("level-sensitive", "4 NAND");

    run("Glitch inside the EN=1 window: survives onto Q", 6, 10);
    run("Same glitch at EN=0: latched, Q does not change", 12, 10);

    T.dump("traces/latch_glitch.js");
}
