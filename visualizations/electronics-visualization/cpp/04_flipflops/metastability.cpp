// Metastability — Violating the Window
//
// A real metastable flip-flop doesn't oscillate cleanly between 0 and 1: its
// analog output parks for an indeterminate time halfway between the two
// logic levels, and that time has no guaranteed upper bound. The -1 level
// already used for the tri-state buffer is handy here as a discrete symbol
// for "not yet either 0 or 1" — it isn't a real third logic state, it's the
// best discrete approximation of a continuous phenomenon.
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/waves.hpp"

static Tracer T("metastability", "Metastability", "Flip-Flops", __FILE__);
static const int N = 20;
static const int EDGE = 8;

static void run(const std::string& label, int resolveAfter, int resolvedTo) {
    std::vector<int> D(N, 0), CLK(N, 0), Q(N, 0);
    for (int t = EDGE; t < N; t++) D[t] = 1;   // D changes right at the edge: a violation
    CLK[EDGE] = 1;

    for (int t = 0; t < N; t++) {
        if (t < EDGE) { Q[t] = 0; continue; }
        if (t < EDGE + resolveAfter) Q[t] = -1;      // metastable: undecided
        else Q[t] = resolvedTo;                       // settled, eventually
    }

    Waves W;
    W.add("D", "data", D);
    W.add("CLK", "clk", CLK);
    W.add("Q", "out", Q);

    T.input(label);
    for (int t = 0; t < N; t++) {
        T.at(__LINE__, __func__)
         .vars({{"t", t}, {"D", D[t]}, {"CLK", CLK[t]}, {"Q", Q[t]}})
         .waveState(W.json(t))
         .note(t < EDGE ? "Before the edge: Q = 0."
               : t == EDGE ? "D changes exactly at the edge: violation, Q enters metastability."
               : Q[t] == -1 ? "Still metastable: neither 0 nor 1, resolution time not guaranteed."
               : "Resolved to Q = " + std::to_string(Q[t]) + ".")
         .emit();
    }
}

int main() {
    T.view("waveform").complexity("edge-triggered", "1 D-FF");

    run("Fast resolution (2 steps)", 2, 1);
    run("Slow resolution (6 steps), same input", 6, 1);

    T.dump("traces/metastability.js");
}
