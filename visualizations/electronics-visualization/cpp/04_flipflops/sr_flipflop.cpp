// SR Flip-Flop — Master-Slave, Rising Edge
//
// Two SR latches in cascade, enabled in antiphase: the MASTER is transparent
// while CLK=0 and tracks S/R; the SLAVE is transparent while CLK=1 and copies
// what the master just froze. The output moves exactly at the instant CLK
// rises from 0 to 1 — not before, not after: that's the difference between a
// "flip-flop" and a "latch". Each individual latch's NOR core is already
// covered in the "SR Latch (NOR)" example; here the level of detail is the
// block.
#include <map>
#include <string>
#include "../tracer/tracer.hpp"
#include "../sim/circuit.hpp"

static Tracer T("sr_flipflop", "SR Flip-Flop", "Flip-Flops", __FILE__);

static Schematic layout() {
    Schematic S;
    S.gate("master", "MS", 2.0, 1.4);
    S.gate("slave", "MS", 4.4, 1.4);
    S.pin("S", 0.0, 0.6, "left");
    S.pin("R", 0.0, 1.4, "left");
    S.pin("CLK", 0.0, 2.4, "left");
    S.wire("S", "master.a");
    S.wire("R", "master.b");
    S.wire("master", "slave.a");
    S.pin("Q", 6.2, 1.0, "right");
    S.pin("Qb", 6.2, 1.8, "right");
    S.wire("slave", "Q");
    return S;
}

// Same simplification as sr_latch_nor for the S=R=1 case: the NOR core forces
// Q=Qb=0 there instead of holding, but a flip-flop is not meant to be driven
// with S=R=1 in the first place, so this entry's inputs never do.
static int srUpdate(int S, int R, int prevQ) {
    if (S) return 1;
    if (R) return 0;
    return prevQ;
}

int main() {
    T.view("schematic").complexity("edge-triggered", "2 latches");

    Schematic sch = layout();
    int Q = 0, masterQ = 0;

    std::vector<int> Sv = {1, 1, 0, 0, 0, 0, 0, 0};
    std::vector<int> Rv = {0, 0, 0, 0, 1, 1, 0, 0};
    std::vector<int> Cv = {0, 1, 0, 1, 0, 1, 0, 1};

    T.input("Set then Reset, one CLK cycle per group");
    for (size_t t = 0; t < Cv.size(); t++) {
        int S = Sv[t], R = Rv[t], CLK = Cv[t];
        std::string active;
        if (CLK == 0) {
            masterQ = srUpdate(S, R, Q);
            active = "master";
        } else {
            Q = masterQ;
            active = "slave";
        }
        std::map<std::string, int> v{{"S", S}, {"R", R}, {"CLK", CLK},
                                      {"master", masterQ}, {"slave", Q}, {"Q", Q}};
        T.at(__LINE__, __func__)
         .vars({{"S", S}, {"R", R}, {"CLK", CLK}, {"masterQ", masterQ}, {"Q", Q}})
         .circuitState(sch.json(v, active))
         .note(CLK == 0
               ? "CLK=0: the master is transparent and tracks S/R -> masterQ = " + std::to_string(masterQ) + "."
               : "CLK=1: the slave copies the just-frozen master -> Q = " + std::to_string(Q) + ".")
         .emit();
    }

    T.dump("traces/sr_flipflop.js");
}
