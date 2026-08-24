// Ripple-Carry, 4 Bit — Watch the Carry Propagate
//
// Each full adder has to wait for the carry-out of the previous one before it
// can produce its own: this is what makes a ripple adder's delay linear in
// the number of bits — exactly what the comparison with carry-look-ahead
// exists to show.
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/logic.hpp"
#include "../sim/circuit.hpp"

static Tracer T("ripple_carry", "Ripple-Carry, 4 bit", "Adders", __FILE__);
static const int NBITS = 4;

// Each full adder is drawn as a single block (its internal gates are the
// subject of full_adder.cpp already); only the carry chain between blocks is
// wired here, which is exactly the thing this entry exists to show.
static Schematic layout() {
    Schematic S;
    for (int i = 0; i < NBITS; i++) S.gate("fa" + std::to_string(i), "FA", 1.0 + i * 2.2, 1.2);
    S.pin("Cin", 0.0, 1.2, "left");
    S.wire("Cin", "fa0.a");
    for (int i = 0; i < NBITS - 1; i++)
        S.wire("fa" + std::to_string(i), "fa" + std::to_string(i + 1) + ".a");
    S.pin("Cout", 1.0 + (NBITS - 1) * 2.2 + 1.6, 1.2, "right");
    S.wire("fa" + std::to_string(NBITS - 1), "Cout");
    return S;
}

void ripple(int aVal, int bVal) {
    std::vector<int> A(NBITS), B(NBITS), S(NBITS, 0);
    for (int i = 0; i < NBITS; i++) { A[i] = (aVal >> i) & 1; B[i] = (bVal >> i) & 1; }

    Schematic sch = layout();
    std::map<std::string, int> v{{"Cin", 0}};
    T.clearWatches();
    T.watchVector("A", A);
    T.watchVector("B", B);
    T.watchVector("S", S);

    int carry = 0;
    for (int i = 0; i < NBITS; i++) {
        int s = XOR(XOR(A[i], B[i]), carry);
        int cout = OR(AND(A[i], B[i]), AND(carry, XOR(A[i], B[i])));
        S[i] = s;
        v["fa" + std::to_string(i)] = cout;
        T.at(__LINE__, __func__)
         .vars({{"bit", i}, {"A_i", A[i]}, {"B_i", B[i]}, {"Cin", carry}, {"S_i", s}, {"Cout_i", cout}})
         .circuitState(sch.json(v, "fa" + std::to_string(i)))
         .note("Bit " + std::to_string(i) + ": must wait for the previous bit's carry (" +
               std::to_string(carry) + "). S=" + std::to_string(s) + ", carry out=" +
               std::to_string(cout) + ".")
         .emit();
        carry = cout;
    }

    v["Cout"] = carry;
    T.at(__LINE__, __func__)
     .vars({{"A", aVal}, {"B", bVal}, {"Sum", aVal + bVal}, {"Cout", carry}})
     .circuitState(sch.json(v))
     .note("Full sum: " + std::to_string(aVal) + " + " + std::to_string(bVal) + " = " +
           std::to_string(aVal + bVal) + " (Cout=" + std::to_string(carry) + ").")
     .emit();
}

int main() {
    T.view("schematic").complexity("4 t_p", "4 FA");
    T.panel("array", "Bit A, B, S");

    T.input("3 + 2 (no carry)");
    ripple(3, 2);
    T.input("7 + 9 (carry propagates through the whole chain)");
    ripple(7, 9);
    T.input("15 + 1 (overflow beyond 4 bits)");
    ripple(15, 1);

    T.dump("traces/ripple_carry.js");
}
