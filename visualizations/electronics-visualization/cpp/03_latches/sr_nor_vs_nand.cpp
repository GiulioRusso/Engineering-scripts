// NOR-Based vs NAND-Based SR — The Duality
//
// Same intent (set / hold / reset / forbidden), two circuits that read it
// oppositely: the NOR core rests at 0 and is commanded with 1 (active-high),
// the NAND core rests at 1 and is commanded with 0 (active-low). Even the
// forbidden state is inverted: NOR forces it to 0,0, NAND to 1,1.
#include <map>
#include <string>
#include "../tracer/tracer.hpp"
#include "../sim/logic.hpp"
#include "../sim/circuit.hpp"

static Tracer T("sr_nor_vs_nand", "NOR vs NAND: Duality", "Latches", __FILE__);

static Schematic layoutNor() {
    Schematic S;
    S.gate("g1", "NOR", 2.2, 0.8);
    S.gate("g2", "NOR", 2.2, 2.2);
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

static Schematic layoutNand() {
    Schematic S;
    S.gate("g1", "NAND", 2.2, 0.8);
    S.gate("g2", "NAND", 2.2, 2.2);
    S.pin("Sb", 0.0, 0.8, "left");
    S.pin("Rb", 0.0, 2.2, "left");
    S.wire("Sb", "g1.a");
    S.wire("g2", "g1.b");
    S.wire("Rb", "g2.a");
    S.wire("g1", "g2.b");
    S.pin("Q", 4.0, 0.8, "right");
    S.pin("Qb", 4.0, 2.2, "right");
    S.wire("g1", "Q");
    S.wire("g2", "Qb");
    return S;
}

// The primary view is always the NOR schematic (unnamed "circuit"); the panel
// is always the NAND schematic (named "nand") - fixed roles, regardless of
// which core is the one actually settling this particular step. vNor/vNand
// persist across every call in main(): whichever side isn't settling still
// needs to render its own last-known state, not a blank one.
void settleNor(Schematic& norSch, std::map<std::string, int>& v, Schematic& nandSch,
               const std::map<std::string, int>& vNand, int S, int R) {
    v["S"] = S;
    v["R"] = R;
    int Q = v["Q"], Qb = v["Qb"];
    for (int pass = 0; pass < 6; pass++) {
        bool changed = false;
        int newQ = NOR(R, Qb);
        bool qDiff = newQ != Q;
        if (pass == 0 || qDiff) {
            Q = newQ;
            v["g1"] = Q;
            v["Q"] = Q;
            T.at(__LINE__, "settleNor")
             .vars({{"S", S}, {"R", R}, {"Q_nor", Q}, {"Qb_nor", Qb}})
             .circuitState(norSch.json(v, "g1"))
             .circuitState("nand", nandSch.json(vNand))
             .note("NOR: g1 -> Q = NOR(R, Qb) = " + std::to_string(Q) + ".")
             .emit();
        }
        changed = changed || qDiff;
        int newQb = NOR(S, Q);
        bool qbDiff = newQb != Qb;
        if (pass == 0 || qbDiff) {
            Qb = newQb;
            v["g2"] = Qb;
            v["Qb"] = Qb;
            T.at(__LINE__, "settleNor")
             .vars({{"S", S}, {"R", R}, {"Q_nor", Q}, {"Qb_nor", Qb}})
             .circuitState(norSch.json(v, "g2"))
             .circuitState("nand", nandSch.json(vNand))
             .note("NOR: g2 -> Qb = NOR(S, Q) = " + std::to_string(Qb) + ".")
             .emit();
        }
        changed = changed || qbDiff;
        if (pass > 0 && !changed) break;
    }
}

void settleNand(Schematic& nandSch, std::map<std::string, int>& v, Schematic& norSch,
                const std::map<std::string, int>& vNor, int Sb, int Rb) {
    v["Sb"] = Sb;
    v["Rb"] = Rb;
    int Q = v["Q"], Qb = v["Qb"];
    for (int pass = 0; pass < 6; pass++) {
        bool changed = false;
        int newQ = NAND(Sb, Qb);
        bool qDiff = newQ != Q;
        if (pass == 0 || qDiff) {
            Q = newQ;
            v["g1"] = Q;
            v["Q"] = Q;
            T.at(__LINE__, "settleNand")
             .vars({{"Sb", Sb}, {"Rb", Rb}, {"Q_nand", Q}, {"Qb_nand", Qb}})
             .circuitState(norSch.json(vNor))
             .circuitState("nand", nandSch.json(v, "g1"))
             .note("NAND: g1 -> Q = NAND(Sb, Qb) = " + std::to_string(Q) + ".")
             .emit();
        }
        changed = changed || qDiff;
        int newQb = NAND(Rb, Q);
        bool qbDiff = newQb != Qb;
        if (pass == 0 || qbDiff) {
            Qb = newQb;
            v["g2"] = Qb;
            v["Qb"] = Qb;
            T.at(__LINE__, "settleNand")
             .vars({{"Sb", Sb}, {"Rb", Rb}, {"Q_nand", Q}, {"Qb_nand", Qb}})
             .circuitState(norSch.json(vNor))
             .circuitState("nand", nandSch.json(v, "g2"))
             .note("NAND: g2 -> Qb = NAND(Rb, Q) = " + std::to_string(Qb) + ".")
             .emit();
        }
        changed = changed || qbDiff;
        if (pass > 0 && !changed) break;
    }
}

int main() {
    T.view("schematic").complexity("level-sensitive", "2+2 gates");
    T.panel("schematic", "NAND (active-low)", "nand");

    Schematic norSch = layoutNor(), nandSch = layoutNand();
    // g1's output IS Q and g2's output IS Qb by construction, in both cores:
    // the g1/g2 keys must be seeded to match, or the very first settle step
    // reads a stale (default-zero) value off the OTHER gate's feedback wire.
    std::map<std::string, int> vNor{{"Q", 0}, {"Qb", 1}, {"g1", 0}, {"g2", 1}};   // NOR core, resting/reset
    std::map<std::string, int> vNand{{"Q", 1}, {"Qb", 0}, {"g1", 1}, {"g2", 0}};  // NAND core, resting/hold

    T.input("SET: NOR S=1 R=0 -- NAND Sb=0 Rb=1");
    settleNor(norSch, vNor, nandSch, vNand, 1, 0);
    settleNand(nandSch, vNand, norSch, vNor, 0, 1);

    T.input("HOLD: NOR S=0 R=0 -- NAND Sb=1 Rb=1");
    settleNor(norSch, vNor, nandSch, vNand, 0, 0);
    settleNand(nandSch, vNand, norSch, vNor, 1, 1);

    T.input("RESET: NOR S=0 R=1 -- NAND Sb=1 Rb=0");
    settleNor(norSch, vNor, nandSch, vNand, 0, 1);
    settleNand(nandSch, vNand, norSch, vNor, 1, 0);

    T.input("FORBIDDEN: NOR S=1 R=1 -- NAND Sb=0 Rb=0");
    settleNor(norSch, vNor, nandSch, vNand, 1, 1);
    settleNand(nandSch, vNand, norSch, vNor, 0, 0);

    T.dump("traces/sr_nor_vs_nand.js");
}
