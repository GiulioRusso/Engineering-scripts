// Setup and Hold — The Sampling Window
//
// D must stay stable for t_su before the edge and t_h after it: that window
// is the price of sampling. Changing D outside the window gives a clean,
// predictable result; changing it inside isn't "wrong" in a logical sense —
// it's undefined, the real circuit no longer guarantees what gets captured.
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/waves.hpp"

static Tracer T("setup_hold", "Setup and Hold", "Flip-Flops", __FILE__);
static const int N = 20;
static const int EDGE = 10, T_SU = 2, T_H = 1;

int main() {
    T.view("waveform").complexity("edge-triggered", "1 D-FF");

    T.input("D changes outside the window: clean capture");
    {
        std::vector<int> D(N, 0), CLK(N, 0);
        for (int t = EDGE - 6; t < N; t++) D[t] = 1;   // stable well before the edge
        CLK[EDGE] = 1;
        Waves W;
        W.add("D", "data", D);
        W.add("CLK", "clk", CLK);
        std::string winJson = "{\"from\":" + std::to_string(EDGE - T_SU) + ",\"to\":" +
                               std::to_string(EDGE + T_H) + ",\"label\":\"t_su + t_h\"}";
        for (int t = 0; t < N; t++) {
            T.at(__LINE__, __func__)
             .vars({{"t", t}, {"D", D[t]}, {"CLK", CLK[t]}})
             .waveState(W.json(t, "[]", winJson))
             .note(t == EDGE ? "Rising edge: D was stable through the whole window, capture guaranteed."
                              : "D stable, outside the window.")
             .emit();
        }
    }

    T.input("D changes inside the window: result not guaranteed");
    {
        std::vector<int> D(N, 0), CLK(N, 0);
        for (int t = EDGE - 1; t < N; t++) D[t] = 1;   // changes INSIDE [EDGE-T_SU, EDGE+T_H]
        CLK[EDGE] = 1;
        Waves W;
        W.add("D", "data", D);
        W.add("CLK", "clk", CLK);
        std::string winJson = "{\"from\":" + std::to_string(EDGE - T_SU) + ",\"to\":" +
                               std::to_string(EDGE + T_H) + ",\"label\":\"t_su + t_h\"}";
        for (int t = 0; t < N; t++) {
            T.at(__LINE__, __func__)
             .vars({{"t", t}, {"D", D[t]}, {"CLK", CLK[t]}})
             .waveState(W.json(t, "[]", winJson))
             .note(t == EDGE ? "Rising edge: D changed inside the setup window. "
                                "The manufacturer does not guarantee the sampled value."
                              : "D inside or near the forbidden window.")
             .emit();
        }
    }

    T.dump("traces/setup_hold.js");
}
