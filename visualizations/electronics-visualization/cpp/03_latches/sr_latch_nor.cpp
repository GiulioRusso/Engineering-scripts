// SR Latch (NOR-Based) — The Cross-Coupled Core
//
// Q = NOR(R, Qb); Qb = NOR(S, Q). Each gate reads the other's output: there is
// no "correct" order to evaluate them in, only a succession of gate delays
// until the circuit stops changing. This is settling, and it genuinely takes
// several gate-delay steps to complete — not a detail to hide, it's the whole
// lesson of this example.
#include <map>
#include <string>
#include "../tracer/tracer.hpp"
#include "../sim/logic.hpp"
#include "../sim/circuit.hpp"

static Tracer T("sr_latch_nor", "SR Latch (NOR)", "Latches", __FILE__);

static Schematic layout() {
    Schematic S;
    S.gate("g1", "NOR", 2.2, 0.8);   // Q
    S.gate("g2", "NOR", 2.2, 2.2);   // Qb
    S.pin("R", 0.0, 0.8, "left");
    S.pin("S", 0.0, 2.2, "left");
    S.wire("R", "g1.a");
    S.wire("g2", "g1.b");
    S.wire("S", "g2.a");
    S.wire("g1", "g2.b");
    S.pin("Q", 4.0, 0.8, "right");
    S.pin("Qb", 4.0, 2.2, "right");
    S.wire("g1", "Q");
    S.wire("g2", "Qb");
    return S;
}

// One step per gate re-evaluation: g1 and g2 alternate, each reading the
// other's most recently recorded output. Pass 0 always emits both (to show
// the starting condition even when nothing changes); later passes emit only
// the gate whose output actually moved, and stop once a whole pass is quiet.
void settle(Schematic& sch, int S, int R, int& Q, int& Qb) {
    std::map<std::string, int> v{{"S", S}, {"R", R}, {"g1", Q}, {"g2", Qb}, {"Q", Q}, {"Qb", Qb}};
    for (int pass = 0; pass < 6; pass++) {
        bool changed = false;

        int newQ = NOR(R, Qb);
        bool qDiff = newQ != Q;
        if (pass == 0 || qDiff) {
            Q = newQ;
            v["g1"] = Q;
            v["Q"] = Q;
            T.at(__LINE__, "settle")
             .vars({{"S", S}, {"R", R}, {"Q", Q}, {"Qb", Qb}})
             .circuitState(sch.json(v, "g1"))
             .note(qDiff ? "g1 re-evaluates: Q = NOR(R, Qb) moves to " + std::to_string(Q) + "."
                         : "g1 re-evaluates: Q = NOR(R, Qb) stays " + std::to_string(Q) + ".")
             .emit();
        }
        changed = changed || qDiff;

        int newQb = NOR(S, Q);
        bool qbDiff = newQb != Qb;
        if (pass == 0 || qbDiff) {
            Qb = newQb;
            v["g2"] = Qb;
            v["Qb"] = Qb;
            T.at(__LINE__, "settle")
             .vars({{"S", S}, {"R", R}, {"Q", Q}, {"Qb", Qb}})
             .circuitState(sch.json(v, "g2"))
             .note(qbDiff ? "g2 re-evaluates: Qb = NOR(S, Q) moves to " + std::to_string(Qb) + "."
                          : "g2 re-evaluates: Qb = NOR(S, Q) stays " + std::to_string(Qb) + ".")
             .emit();
        }
        changed = changed || qbDiff;

        if (pass > 0 && !changed) break;
    }
}

int main() {
    T.view("schematic").complexity("level-sensitive", "2 NOR");

    Schematic sch = layout();
    int Q = 0, Qb = 1;  // assumed initial state: reset

    T.input("SET: S=1 R=0");
    settle(sch, 1, 0, Q, Qb);
    T.input("HOLD: S=0 R=0");
    settle(sch, 0, 0, Q, Qb);
    T.input("RESET: S=0 R=1");
    settle(sch, 0, 1, Q, Qb);
    T.input("FORBIDDEN: S=1 R=1");
    settle(sch, 1, 1, Q, Qb);

    T.dump("traces/sr_latch_nor.js");
}
