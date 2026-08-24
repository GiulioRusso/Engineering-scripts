// Same Input, Different Outputs — Moore Lags by One Cycle
//
// The '1'-detector machine from the Moore/Mealy comparison, driven by the
// same bit stream: the Mealy output reacts in the same cycle as the input
// that caused it, the Moore output waits for the state to update and so
// arrives one cycle later. On the same graph, the gap is a single step to
// the right.
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/waves.hpp"

static Tracer T("moore_mealy_timing", "Same Input, Different Outputs", "FSM", __FILE__);

int main() {
    T.view("waveform").complexity("O(1) per symbol", "2 states");

    std::string bits = "0011010010";
    int N = static_cast<int>(bits.size());
    std::vector<int> in(N, 0), outMealy(N, 0), outMoore(N, 0);

    for (int i = 0; i < N; i++) {
        int bit = bits[i] - '0';
        in[i] = bit;
        outMealy[i] = bit;                    // Mealy: combinational on the input, same cycle
        outMoore[i] = i > 0 ? in[i - 1] : 0;  // Moore: reflects the PREVIOUS cycle's state
    }

    Waves W;
    W.add("in", "data", in);
    W.add("out_mealy", "out", outMealy);
    W.add("out_moore", "out", outMoore);

    T.input("Same bit stream on both machines");
    for (int i = 0; i < N; i++) {
        T.at(__LINE__, __func__)
         .vars({{"t", i}, {"in", in[i]}, {"out_mealy", outMealy[i]}, {"out_moore", outMoore[i]}})
         .waveState(W.json(i))
         .note("out_mealy follows the input right away; out_moore is still showing the response to the previous bit.")
         .emit();
    }

    T.dump("traces/moore_mealy_timing.js");
}
