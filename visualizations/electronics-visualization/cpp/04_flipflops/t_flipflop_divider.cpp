// T Flip-Flop as a Frequency Divider
//
// T = J = K = 1 always: every rising edge of its own clock flips the output,
// so the output has half the frequency of the input. Chaining three stages,
// each driven by the previous one's output, gives /2, /4, /8 of the same
// starting clock — a tiny binary counter.
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/waves.hpp"

static Tracer T("t_flipflop_divider", "T as a Frequency Divider", "Flip-Flops", __FILE__);
static const int N = 32;

// Master-slave toggle: master tracks NOT(Q) while its own clock is low,
// slave commits it to Q when that clock rises - one toggle per rising edge.
static std::vector<int> toggleChain(const std::vector<int>& clk) {
    std::vector<int> out(clk.size(), 0);
    int Q = 0, masterQ = 0;
    for (size_t t = 0; t < clk.size(); t++) {
        if (clk[t] == 0) masterQ = 1 - Q; else Q = masterQ;
        out[t] = Q;
    }
    return out;
}

int main() {
    T.view("waveform").complexity("edge-triggered", "3 T-FF");

    std::vector<int> CLK(N);
    for (int t = 0; t < N; t++) CLK[t] = (t / 2) % 2;
    std::vector<int> Q1 = toggleChain(CLK);
    std::vector<int> Q2 = toggleChain(Q1);
    std::vector<int> Q3 = toggleChain(Q2);

    Waves W;
    W.add("CLK", "clk", CLK);
    W.add("Q1 (/2)", "out", Q1);
    W.add("Q2 (/4)", "out", Q2);
    W.add("Q3 (/8)", "out", Q3);

    T.input("Three stages in cascade");
    for (int t = 0; t < N; t++) {
        T.at(__LINE__, __func__)
         .vars({{"t", t}, {"CLK", CLK[t]}, {"Q1", Q1[t]}, {"Q2", Q2[t]}, {"Q3", Q3[t]}})
         .waveState(W.json(t))
         .note("Q1 toggles at half CLK's frequency, Q2 at half of Q1's, Q3 at half of Q2's.")
         .emit();
    }

    T.dump("traces/t_flipflop_divider.js");
}
