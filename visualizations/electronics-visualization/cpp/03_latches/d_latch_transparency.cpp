// D Latch Transparency — Why It's "Level-Sensitive"
//
// As long as EN=1 the latch is transparent: Q copies D instant by instant,
// even several times within the same enable pulse. Only when EN falls does Q
// latch onto the last value seen. This — sensitive to the LEVEL of EN, not
// one of its edges — is what tells a latch apart from a flip-flop.
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/logic.hpp"
#include "../sim/waves.hpp"

static Tracer T("d_latch_transparency", "Transparency", "Latches", __FILE__);
static const int N = 24;

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

int main() {
    T.view("waveform").complexity("level-sensitive", "4 NAND");

    std::vector<int> D(N, 0), EN(N, 0), Qv(N);
    for (int t : {2, 3, 4, 5, 6, 7}) EN[t] = 1;
    for (int t : {14, 15, 16, 17}) EN[t] = 1;
    for (int t : {3, 4, 8, 9, 15, 16, 20, 21}) D[t] = 1;

    int Q = 0, Qb = 1;
    for (int t = 0; t < N; t++) {
        settleQuiet(D[t], EN[t], Q, Qb);
        Qv[t] = Q;
    }

    Waves W;
    W.add("D", "data", D);
    W.add("EN", "data", EN);
    W.add("Q", "out", Qv);

    T.input("D changes both inside and outside the EN pulse");
    for (int t = 0; t < N; t++) {
        T.at(__LINE__, __func__)
         .vars({{"t", t}, {"D", D[t]}, {"EN", EN[t]}, {"Q", Qv[t]}})
         .waveState(W.json(t))
         .note(EN[t] ? "EN=1: transparent, Q follows D = " + std::to_string(D[t]) + "."
                     : "EN=0: latched, Q stays " + std::to_string(Qv[t]) +
                       " no matter what D does.")
         .emit();
    }

    T.dump("traces/d_latch_transparency.js");
}
