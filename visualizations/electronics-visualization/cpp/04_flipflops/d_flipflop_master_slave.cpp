// D Flip-Flop Master-Slave — Follow the Schematic Through Time
//
// Master transparent while CLK=0 (tracks D), slave transparent while CLK=1
// (copies the master). The result: Q changes only at CLK's rising edge, to
// whatever value D had right at that instant — never mid-level, unlike the D
// latch from the previous chapter.
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/circuit.hpp"
#include "../sim/waves.hpp"

static Tracer T("d_flipflop_master_slave", "D Master-Slave", "Flip-Flops", __FILE__);
static const int N = 20;

static Schematic layout() {
    Schematic S;
    S.gate("master", "MS", 2.0, 1.0);
    S.gate("slave", "MS", 4.4, 1.0);
    S.pin("D", 0.0, 0.6, "left");
    S.pin("CLK", 0.0, 1.8, "left");
    S.wire("D", "master.a");
    S.wire("master", "slave.a");
    S.pin("Q", 6.2, 1.0, "right");
    S.wire("slave", "Q");
    return S;
}

int main() {
    T.view("waveform").complexity("edge-triggered", "2 D-latches");
    T.panel("schematic", "Schematic");

    std::vector<int> D(N, 0), CLK(N), Qv(N), masterQv(N);
    // Kept off the CLK edges on purpose: this entry is about master-slave
    // sampling working cleanly, not about setup-time violations (that is
    // setup_hold.cpp's job). Rising edges land at t=2,6,10,14,18.
    for (int t : {3, 4, 5, 11, 12, 13, 17}) D[t] = 1;
    for (int t = 0; t < N; t++) CLK[t] = (t / 2) % 2;

    Schematic sch = layout();
    int Q = 0, masterQ = 0;

    T.input("D changes during both CLK phases");
    for (int t = 0; t < N; t++) {
        std::string active;
        if (CLK[t] == 0) {
            masterQ = D[t];
            active = "master";
        } else {
            Q = masterQ;
            active = "slave";
        }
        masterQv[t] = masterQ;
        Qv[t] = Q;

        Waves W;
        W.add("D", "data", D);
        W.add("CLK", "clk", CLK);
        W.add("Q", "out", Qv);

        std::map<std::string, int> v{{"D", D[t]}, {"CLK", CLK[t]}, {"master", masterQ}, {"slave", Q}, {"Q", Q}};
        T.at(__LINE__, __func__)
         .vars({{"t", t}, {"D", D[t]}, {"CLK", CLK[t]}, {"masterQ", masterQ}, {"Q", Q}})
         .waveState(W.json(t))
         .circuitState(sch.json(v, active))
         .note(CLK[t] == 0 ? "CLK=0: master transparent, tracks D = " + std::to_string(D[t]) + "."
                           : "CLK=1: slave copies the master -> Q = " + std::to_string(Q) + ".")
         .emit();
    }

    T.dump("traces/d_flipflop_master_slave.js");
}
