// Parity Generator — A Tree of XORs
//
// XOR is the parity gate: its output is 1 exactly when an odd number of its
// inputs are 1. A tree of XORs extends the idea to a whole word in log2(n)
// levels, instead of a linear chain of n-1 gates.
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/logic.hpp"
#include "../sim/circuit.hpp"

static Tracer T("parity_tree", "Parity Generator", "Adders", __FILE__);

static Schematic layout() {
    Schematic S;
    S.gate("g1", "XOR", 2.2, 0.6);   // w0 xor w1
    S.gate("g2", "XOR", 2.2, 2.2);   // w2 xor w3
    S.gate("g3", "XOR", 4.4, 1.4);   // parity
    S.pin("w0", 0.0, 0.2, "left");
    S.pin("w1", 0.0, 1.0, "left");
    S.pin("w2", 0.0, 1.8, "left");
    S.pin("w3", 0.0, 2.6, "left");
    S.wire("w0", "g1.a");
    S.wire("w1", "g1.b");
    S.wire("w2", "g2.a");
    S.wire("w3", "g2.b");
    S.wire("g1", "g3.a");
    S.wire("g2", "g3.b");
    S.pin("PARITY", 6.2, 1.4, "right");
    S.wire("g3", "PARITY");
    return S;
}

void parity(int w0, int w1, int w2, int w3) {
    Schematic sch = layout();
    int p1 = XOR(w0, w1), p2 = XOR(w2, w3), par = XOR(p1, p2);
    std::map<std::string, int> v{{"w0", w0}, {"w1", w1}, {"w2", w2}, {"w3", w3},
                                  {"g1", p1}, {"g2", p2}, {"g3", par}, {"PARITY", par}};
    T.at(__LINE__, __func__)
     .vars({{"w0", w0}, {"w1", w1}, {"w2", w2}, {"w3", w3}, {"PARITY", par}})
     .circuitState(sch.json(v, "g1"))
     .note("Level 1, left pair: w0 XOR w1 = " + std::to_string(p1) + ".")
     .emit();
    T.at(__LINE__, __func__)
     .vars({{"w0", w0}, {"w1", w1}, {"w2", w2}, {"w3", w3}, {"PARITY", par}})
     .circuitState(sch.json(v, "g2"))
     .note("Level 1, right pair: w2 XOR w3 = " + std::to_string(p2) + ".")
     .emit();
    T.at(__LINE__, __func__)
     .vars({{"w0", w0}, {"w1", w1}, {"w2", w2}, {"w3", w3}, {"PARITY", par}})
     .circuitState(sch.json(v, "g3"))
     .note("Level 2, root: parity = " + std::to_string(p1) + " XOR " + std::to_string(p2) +
           " = " + std::to_string(par) + " (" + (par ? "odd" : "even") + " number of 1s).")
     .emit();
}

int main() {
    T.view("schematic").complexity("2 t_p", "3 XOR");

    T.input("0000 (even)");
    parity(0, 0, 0, 0);
    T.input("1000 (odd)");
    parity(1, 0, 0, 0);
    T.input("1100 (even)");
    parity(1, 1, 0, 0);
    T.input("1110 (odd)");
    parity(1, 1, 1, 0);
    T.input("1111 (even)");
    parity(1, 1, 1, 1);

    T.dump("traces/parity_tree.js");
}
