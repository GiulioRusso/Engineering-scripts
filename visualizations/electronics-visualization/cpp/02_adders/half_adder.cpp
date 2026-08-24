// Half Adder — One XOR, One AND
//
// S = A XOR B, Cout = A AND B. The two gates share both inputs and neither
// depends on the other's output — which is exactly why a half adder cannot
// accept a carry-in: there is nowhere for one to arrive.
#include <map>
#include <string>
#include "../tracer/tracer.hpp"
#include "../sim/logic.hpp"
#include "../sim/circuit.hpp"

static Tracer T("half_adder", "Half Adder", "Adders", __FILE__);

static Schematic layout() {
    Schematic S;
    S.gate("g1", "XOR", 2.2, 1.0);
    S.gate("g2", "AND", 2.2, 2.2);
    S.pin("A", 0.0, 0.7, "left");
    S.pin("B", 0.0, 2.5, "left");
    S.wire("A", "g1.a");
    S.wire("A", "g2.a");
    S.wire("B", "g1.b");
    S.wire("B", "g2.b");
    S.pin("S", 4.0, 1.0, "right");
    S.pin("Cout", 4.0, 2.2, "right");
    S.wire("g1", "S");
    S.wire("g2", "Cout");
    return S;
}

// Combinational logic is instantaneous: both gate outputs already hold their
// final value from the first step. What changes step to step is only which
// gate the explanation is pointing at, never a bit.
void halfAdder(int A, int B) {
    Schematic sch = layout();
    int S = XOR(A, B);
    int Cout = AND(A, B);
    std::map<std::string, int> v{{"A", A}, {"B", B}, {"g1", S}, {"S", S}, {"g2", Cout}, {"Cout", Cout}};
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"S", S}, {"Cout", Cout}})
     .circuitState(sch.json(v))
     .note("Inputs applied: A=" + std::to_string(A) + ", B=" + std::to_string(B) + ".")
     .emit();
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"S", S}, {"Cout", Cout}})
     .circuitState(sch.json(v, "g1"))
     .note("The XOR gate computes the sum: S = A XOR B = " + std::to_string(S) + ".")
     .emit();
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"S", S}, {"Cout", Cout}})
     .circuitState(sch.json(v, "g2"))
     .note("The AND gate computes the carry: Cout = A AND B = " + std::to_string(Cout) + ".")
     .emit();
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"S", S}, {"Cout", Cout}})
     .circuitState(sch.json(v))
     .note("Result: " + std::to_string(A) + " + " + std::to_string(B) + " = " +
           std::to_string(Cout) + std::to_string(S) + " in binary.")
     .emit();
}

int main() {
    T.view("schematic").complexity("2 t_p", "2 gate");

    int combos[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    for (auto& c : combos) {
        T.input("A=" + std::to_string(c[0]) + " B=" + std::to_string(c[1]));
        halfAdder(c[0], c[1]);
    }

    T.dump("traces/half_adder.js");
}
