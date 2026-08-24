// NAND Is Universal — NOT, AND and OR, Built From NAND Alone
//
// not_from_nand(a) = NAND(a, a); and_from_nand(a, b) = NAND(n, n) where
// n = NAND(a, b); or_from_nand(a, b) = NAND(NAND(a, a), NAND(b, b)). Every
// other gate is redundant once NAND exists.
#include <map>
#include <string>
#include "../tracer/tracer.hpp"
#include "../sim/logic.hpp"
#include "../sim/circuit.hpp"

static Tracer T("nand_universal", "NAND Is Universal", "Logic Gates", __FILE__);

void notFromNand(int A) {
    Schematic sch;
    sch.gate("g1", "NAND", 2.2, 1.0);
    sch.pin("A", 0.0, 1.0, "left");
    sch.wire("A", "g1.a");
    sch.wire("A", "g1.b");
    sch.pin("OUT", 4.0, 1.0, "right");
    sch.wire("g1", "OUT");

    int out = NAND(A, A);
    std::map<std::string, int> v{{"A", A}, {"g1", out}, {"OUT", out}};
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"OUT", out}})
     .circuitState(sch.json(v, "g1"))
     .note("NOT(A) = NAND(A, A). A single NAND with both inputs tied together: " +
           std::to_string(A) + " -> " + std::to_string(out) + ".")
     .emit();
}

void andFromNand(int A, int B) {
    Schematic sch;
    sch.gate("g1", "NAND", 1.6, 1.0);
    sch.gate("g2", "NAND", 3.8, 1.0);
    sch.pin("A", 0.0, 0.6, "left");
    sch.pin("B", 0.0, 1.4, "left");
    sch.wire("A", "g1.a");
    sch.wire("B", "g1.b");
    sch.wire("g1", "g2.a");
    sch.wire("g1", "g2.b");
    sch.pin("OUT", 5.6, 1.0, "right");
    sch.wire("g2", "OUT");

    int n = NAND(A, B);
    int out = NAND(n, n);
    std::map<std::string, int> v{{"A", A}, {"B", B}, {"g1", n}, {"g2", out}, {"OUT", out}};
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"n", n}, {"OUT", out}})
     .circuitState(sch.json(v, "g1"))
     .note("First NAND: n = NAND(A, B) = " + std::to_string(n) + ".")
     .emit();
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"n", n}, {"OUT", out}})
     .circuitState(sch.json(v, "g2"))
     .note("Second NAND, n against itself: NAND(n, n) = NOT(n) = " + std::to_string(out) +
           " -- which is exactly A AND B.")
     .emit();
}

void orFromNand(int A, int B) {
    Schematic sch;
    sch.gate("g1", "NAND", 1.6, 0.6);
    sch.gate("g2", "NAND", 1.6, 1.6);
    sch.gate("g3", "NAND", 3.8, 1.1);
    sch.pin("A", 0.0, 0.6, "left");
    sch.pin("B", 0.0, 1.6, "left");
    sch.wire("A", "g1.a");
    sch.wire("A", "g1.b");
    sch.wire("B", "g2.a");
    sch.wire("B", "g2.b");
    sch.wire("g1", "g3.a");
    sch.wire("g2", "g3.b");
    sch.pin("OUT", 5.6, 1.1, "right");
    sch.wire("g3", "OUT");

    int na = NAND(A, A);   // NOT A
    int nb = NAND(B, B);   // NOT B
    int out = NAND(na, nb);
    std::map<std::string, int> v{{"A", A}, {"B", B}, {"g1", na}, {"g2", nb}, {"g3", out}, {"OUT", out}};
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"notA", na}, {"notB", nb}, {"OUT", out}})
     .circuitState(sch.json(v, "g1"))
     .note("NAND(A, A) = NOT A = " + std::to_string(na) + ".")
     .emit();
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"notA", na}, {"notB", nb}, {"OUT", out}})
     .circuitState(sch.json(v, "g2"))
     .note("NAND(B, B) = NOT B = " + std::to_string(nb) + ".")
     .emit();
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"notA", na}, {"notB", nb}, {"OUT", out}})
     .circuitState(sch.json(v, "g3"))
     .note("NAND(NOT A, NOT B) = A OR B (De Morgan) = " + std::to_string(out) + ".")
     .emit();
}

int main() {
    T.view("schematic").complexity("2 t_p", "3 NAND");

    T.input("NOT: A=0");
    notFromNand(0);
    T.input("NOT: A=1");
    notFromNand(1);
    T.input("AND: A=1 B=1");
    andFromNand(1, 1);
    T.input("AND: A=1 B=0");
    andFromNand(1, 0);
    T.input("OR: A=0 B=1");
    orFromNand(0, 1);
    T.input("OR: A=0 B=0");
    orFromNand(0, 0);

    T.dump("traces/nand_universal.js");
}
