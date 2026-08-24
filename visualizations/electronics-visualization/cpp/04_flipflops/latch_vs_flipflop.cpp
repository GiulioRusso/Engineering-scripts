// Latch vs Flip-Flop — The Decisive Plot
//
// Same D and CLK, two outputs side by side: Q_latch follows D for the whole
// duration of EN=1 (level-sensitive), Q_ff moves exactly once per cycle,
// right at the rising edge (edge-sensitive). Every change of D inside the
// latch's high window is visible on Q_latch and invisible on Q_ff, unless it
// happens exactly at the edge instant.
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/waves.hpp"

static Tracer T("latch_vs_flipflop", "Latch vs Flip-Flop", "Flip-Flops", __FILE__);
static const int N = 20;

int main() {
    T.view("waveform").complexity("—", "—");

    std::vector<int> D(N, 0), CLK(N), Qlatch(N), Qff(N);
    // Kept off the CLK edges on purpose, same reasoning as
    // d_flipflop_master_slave.cpp. Rising edges land at t=2,6,10,14,18.
    for (int t : {3, 8, 9, 11, 12, 16}) D[t] = 1;
    for (int t = 0; t < N; t++) CLK[t] = (t / 2) % 2;

    int qFF = 0, masterQ = 0;
    for (int t = 0; t < N; t++) {
        // latch: level-sensitive, transparent whenever CLK=1
        Qlatch[t] = CLK[t] ? D[t] : (t > 0 ? Qlatch[t - 1] : 0);
        // flip-flop: master-slave, output moves only at the CLK rising edge
        if (CLK[t] == 0) masterQ = D[t]; else qFF = masterQ;
        Qff[t] = qFF;
    }

    Waves W;
    W.add("D", "data", D);
    W.add("CLK", "clk", CLK);
    W.add("Q_latch", "out", Qlatch);
    W.add("Q_ff", "out", Qff);

    T.input("Same D and CLK, two different cores");
    for (int t = 0; t < N; t++) {
        T.at(__LINE__, __func__)
         .vars({{"t", t}, {"D", D[t]}, {"CLK", CLK[t]}, {"Q_latch", Qlatch[t]}, {"Q_ff", Qff[t]}})
         .waveState(W.json(t))
         .note(CLK[t]
               ? "CLK=1: Q_latch follows D in real time (=" + std::to_string(Qlatch[t]) +
                 "); Q_ff stays fixed at " + std::to_string(Qff[t]) + " until the next edge."
               : "CLK=0: Q_latch is latched; the flip-flop's master is quietly tracking D.")
         .emit();
    }

    T.dump("traces/latch_vs_flipflop.js");
}
