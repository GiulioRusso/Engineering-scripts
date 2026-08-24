// Asynchronous Ripple Counter — The Delay Accumulates
//
// Each stage is driven by the output of the previous stage, not by a common
// clock: when bit0 changes, bit1 only sees it after its own propagation
// delay, bit2 only sees it after seeing bit1 change, and so on. Going from
// 0111 to 1000 is not a clean jump: it passes through 0110, 0100, 0000
// before landing on 1000 — an unreadable value until the ripple settles.
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/circuit.hpp"
#include "../sim/waves.hpp"

static Tracer T("ripple_counter", "Asynchronous Ripple Counter", "Counters", __FILE__);
static const int NBITS = 4;
static const int DELAY = 2;   // sub-ticks of propagation delay per stage

static std::vector<int> toggleChain(const std::vector<int>& clk) {
    std::vector<int> out(clk.size(), 0);
    int Q = 0, masterQ = 0;
    for (size_t t = 0; t < clk.size(); t++) {
        if (clk[t] == 0) masterQ = 1 - Q; else Q = masterQ;
        out[t] = Q;
    }
    return out;
}

// Pads with the signal's OWN starting value, not a hardcoded 0: sig may
// legitimately start at 1 (as an inverted output does), and padding with 0
// there would fabricate a rising edge out of nothing at t=0.
static std::vector<int> delayShift(const std::vector<int>& sig, int delay) {
    std::vector<int> out(sig.size());
    for (size_t t = 0; t < sig.size(); t++) {
        int src = static_cast<int>(t) - delay;
        out[t] = src >= 0 ? sig[src] : sig[0];
    }
    return out;
}

static Schematic layout() {
    Schematic S;
    for (int i = 0; i < NBITS; i++) S.gate("ff" + std::to_string(i), "FF", 1.0 + i * 2.2, 1.2);
    S.pin("CLK", 0.0, 1.2, "left");
    S.wire("CLK", "ff0.a");
    for (int i = 0; i < NBITS - 1; i++)
        S.wire("ff" + std::to_string(i), "ff" + std::to_string(i + 1) + ".a");
    return S;
}

int main() {
    T.view("schematic").complexity("O(n) delay", "4 T-FF");
    T.panel("waveform", "CLK, bits and count");

    const int PERIOD = 12, N = 60;
    std::vector<int> CLK(N);
    for (int t = 0; t < N; t++) CLK[t] = (t / (PERIOD / 2)) % 2;

    // Binary carry propagation needs bit(i+1) to toggle when bit(i) FALLS
    // (1->0), not when it rises: that is the moment a real +1 would carry
    // into the next place. toggleChain triggers on a rising edge, so each
    // stage is clocked by the COMPLEMENT of the previous stage's output.
    auto invert = [](const std::vector<int>& s) {
        std::vector<int> out(s.size());
        for (size_t i = 0; i < s.size(); i++) out[i] = 1 - s[i];
        return out;
    };
    std::vector<int> Q0 = toggleChain(CLK);
    std::vector<int> Q1 = toggleChain(delayShift(invert(Q0), DELAY));
    std::vector<int> Q2 = toggleChain(delayShift(invert(Q1), DELAY));
    std::vector<int> Q3 = toggleChain(delayShift(invert(Q2), DELAY));

    Waves W;
    W.add("CLK", "clk", CLK);
    W.add("bit0", "out", Q0);
    W.add("bit1", "out", Q1);
    W.add("bit2", "out", Q2);
    W.add("bit3", "out", Q3);

    Schematic sch = layout();
    T.input("Free-running count, 2 sub-tick delay per stage");
    for (int t = 0; t < N; t++) {
        int count = Q3[t] * 8 + Q2[t] * 4 + Q1[t] * 2 + Q0[t];
        int prevCount = t > 0 ? Q3[t-1]*8 + Q2[t-1]*4 + Q1[t-1]*2 + Q0[t-1] : count;
        bool rippling = (Q0[t] != (t > 0 ? Q0[t-1] : Q0[t])) ||
                         (Q1[t] != (t > 0 ? Q1[t-1] : Q1[t])) ||
                         (Q2[t] != (t > 0 ? Q2[t-1] : Q2[t])) ||
                         (Q3[t] != (t > 0 ? Q3[t-1] : Q3[t]));
        std::string active = Q0[t] != (t > 0 ? Q0[t-1] : -1) ? "ff0"
                            : Q1[t] != (t > 0 ? Q1[t-1] : -1) ? "ff1"
                            : Q2[t] != (t > 0 ? Q2[t-1] : -1) ? "ff2"
                            : Q3[t] != (t > 0 ? Q3[t-1] : -1) ? "ff3" : "";
        std::map<std::string, int> v{{"CLK", CLK[t]}, {"ff0", Q0[t]}, {"ff1", Q1[t]}, {"ff2", Q2[t]}, {"ff3", Q3[t]}};
        T.at(__LINE__, __func__)
         .vars({{"t", t}, {"CLK", CLK[t]}, {"count", count}})
         .circuitState(sch.json(v, active))
         .waveState(W.json(t))
         .note(rippling && count != prevCount + 1 && !(prevCount == 15 && count == 0)
               ? "In transit: the ripple hasn't settled yet, the value " + std::to_string(count) +
                 " is not the final count."
               : "count = " + std::to_string(count) + ".")
         .emit();
    }

    T.dump("traces/ripple_counter.js");
}
