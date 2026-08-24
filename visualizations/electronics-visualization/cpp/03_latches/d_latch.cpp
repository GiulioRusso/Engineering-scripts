// D Latch with Enable — Transparency Controlled by EN
//
// Two NANDs (g1, g2) turn D and EN into the two active-low commands of a
// cross-coupled SR-NAND core (g3, g4): with EN=0 both commands go to 1 and
// the core holds its state; with EN=1 D passes straight through. The core
// settles exactly like the SR Latch (NOR) example, just with NAND instead of
// NOR.
#include <map>
#include <string>
#include "../tracer/tracer.hpp"
#include "../sim/logic.hpp"
#include "../sim/circuit.hpp"

static Tracer T("d_latch", "D Latch with Enable", "Latches", __FILE__);

static Schematic layout() {
    Schematic S;
    S.gate("g1", "NAND", 1.6, 0.5);   // Sb = NAND(D, EN)
    S.gate("g2", "NAND", 1.6, 1.5);   // Rb = NAND(g1, EN)
    S.gate("g3", "NAND", 3.8, 0.8);   // Q  = NAND(g1, g4)
    S.gate("g4", "NAND", 3.8, 2.0);   // Qb = NAND(g2, g3)
    S.pin("D", 0.0, 0.3, "left");
    S.pin("EN", 0.0, 1.7, "left");
    S.wire("D", "g1.a");
    S.wire("EN", "g1.b");
    S.wire("g1", "g2.a");
    S.wire("EN", "g2.b");
    S.wire("g1", "g3.a");
    S.wire("g4", "g3.b");
    S.wire("g2", "g4.a");
    S.wire("g3", "g4.b");
    S.pin("Q", 5.6, 0.8, "right");
    S.pin("Qb", 5.6, 2.0, "right");
    S.wire("g3", "Q");
    S.wire("g4", "Qb");
    return S;
}

// g3/g4 settle exactly like sr_latch_nor.cpp's g1/g2, only NAND-driven and
// commanded by Sb/Rb (g1/g2's outputs) instead of raw S/R.
void settleCore(Schematic& sch, int D, int EN, int Sb, int Rb, int& Q, int& Qb) {
    std::map<std::string, int> v{{"D", D}, {"EN", EN}, {"g1", Sb}, {"g2", Rb},
                                  {"g3", Q}, {"g4", Qb}, {"Q", Q}, {"Qb", Qb}};
    for (int pass = 0; pass < 6; pass++) {
        bool changed = false;

        int newQ = NAND(Sb, Qb);
        bool qDiff = newQ != Q;
        if (pass == 0 || qDiff) {
            Q = newQ;
            v["g3"] = Q;
            v["Q"] = Q;
            T.at(__LINE__, "settleCore")
             .vars({{"D", D}, {"EN", EN}, {"Q", Q}, {"Qb", Qb}})
             .circuitState(sch.json(v, "g3"))
             .note(qDiff ? "g3 re-evaluates: Q = NAND(Sb, Qb) moves to " + std::to_string(Q) + "."
                         : "g3 re-evaluates: Q = NAND(Sb, Qb) stays " + std::to_string(Q) + ".")
             .emit();
        }
        changed = changed || qDiff;

        int newQb = NAND(Rb, Q);
        bool qbDiff = newQb != Qb;
        if (pass == 0 || qbDiff) {
            Qb = newQb;
            v["g4"] = Qb;
            v["Qb"] = Qb;
            T.at(__LINE__, "settleCore")
             .vars({{"D", D}, {"EN", EN}, {"Q", Q}, {"Qb", Qb}})
             .circuitState(sch.json(v, "g4"))
             .note(qbDiff ? "g4 re-evaluates: Qb = NAND(Rb, Q) moves to " + std::to_string(Qb) + "."
                          : "g4 re-evaluates: Qb = NAND(Rb, Q) stays " + std::to_string(Qb) + ".")
             .emit();
        }
        changed = changed || qbDiff;

        if (pass > 0 && !changed) break;
    }
}

void applyDEN(int D, int EN, int& Q, int& Qb) {
    Schematic sch = layout();
    // g1 and g2 are feedforward (no cycle through them): combinational logic
    // is instantaneous, so both are computed before the first emit and only
    // the highlighted gate changes between these two narration steps.
    int Sb = NAND(D, EN);
    int Rb = NAND(Sb, EN);
    std::map<std::string, int> v{{"D", D}, {"EN", EN}, {"g1", Sb}, {"g2", Rb}, {"Q", Q}, {"Qb", Qb}};
    T.at(__LINE__, __func__)
     .vars({{"D", D}, {"EN", EN}, {"Sb", Sb}, {"Rb", Rb}})
     .circuitState(sch.json(v, "g1"))
     .note("Sb = NAND(D, EN) = " + std::to_string(Sb) + ".")
     .emit();
    T.at(__LINE__, __func__)
     .vars({{"D", D}, {"EN", EN}, {"Sb", Sb}, {"Rb", Rb}})
     .circuitState(sch.json(v, "g2"))
     .note("Rb = NAND(Sb, EN) = " + std::to_string(Rb) + ".")
     .emit();

    settleCore(sch, D, EN, Sb, Rb, Q, Qb);
}

int main() {
    T.view("schematic").complexity("level-sensitive", "4 NAND");

    int Q = 0, Qb = 1;

    T.input("EN=1 D=1 (transparent, Q->1)");
    applyDEN(1, 1, Q, Qb);
    T.input("EN=1 D=0 (transparent, Q->0)");
    applyDEN(0, 1, Q, Qb);
    T.input("EN=0 D=1 (latched, Q unchanged)");
    applyDEN(1, 0, Q, Qb);

    T.dump("traces/d_latch.js");
}
