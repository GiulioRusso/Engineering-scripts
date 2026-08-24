// Ring and Johnson — Shift Registers With Feedback
//
// A ring counter closes the last bit back onto the first without inverting
// it: a single 1 goes around in a circle, N states. A Johnson counter
// inverts that bit before closing the loop: the sequence takes twice as long
// to repeat, 2N states, because it takes N steps to fill the register with
// 1s and another N to empty it out again.
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/waves.hpp"

static Tracer T("ring_johnson", "Ring and Johnson", "Counters", __FILE__);
static const int NBITS = 4;

void run(const std::string& label, bool johnson, int steps) {
    std::vector<int> reg(NBITS, 0);
    if (!johnson) reg[0] = 1;   // ring starts one-hot; Johnson starts all-zero

    std::vector<std::vector<int>> history(NBITS, std::vector<int>(steps, 0));
    for (int i = 0; i < NBITS; i++) history[i][0] = reg[i];
    T.clearWatches();
    T.watchVector("reg", reg);
    T.input(label);
    T.at(__LINE__, "run")
     .vars({{"step", 0}})
     .note("Initial state.")
     .emit();

    for (int s = 1; s < steps; s++) {
        int feedback = johnson ? 1 - reg[NBITS - 1] : reg[NBITS - 1];
        for (int i = NBITS - 1; i > 0; i--) reg[i] = reg[i - 1];
        reg[0] = feedback;
        for (int i = 0; i < NBITS; i++) history[i][s] = reg[i];

        Waves W;
        for (int i = 0; i < NBITS; i++) W.add("bit" + std::to_string(i), "data", history[i]);
        T.at(__LINE__, "run")
         .vars({{"step", s}, {"feedback", feedback}})
         .waveState(W.json(s))
         .note("Shift: the incoming bit becomes " + std::string(johnson ? "NOT(last bit) = " : "last bit = ") +
               std::to_string(feedback) + ".")
         .emit();
    }
}

int main() {
    T.view("array").complexity("O(1) per step", "4 flip-flops");
    T.panel("waveform", "Every bit over time");

    run("Ring (4 states)", false, 9);
    run("Johnson (8 states)", true, 17);

    T.dump("traces/ring_johnson.js");
}
