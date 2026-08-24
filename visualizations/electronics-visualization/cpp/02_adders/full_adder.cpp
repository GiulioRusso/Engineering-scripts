// Full Adder — Two Half Adders Plus an OR
//
// S = A XOR B XOR Cin; Cout = (A AND B) OR (Cin AND (A XOR B)). Cin is what
// the half adder didn't have: this is what makes a ripple chain possible.
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/logic.hpp"
#include "../sim/circuit.hpp"
#include "../sim/truth.hpp"

static Tracer T("full_adder", "Full Adder", "Adders", __FILE__);

static Schematic layout() {
    Schematic S;
    S.gate("g1", "XOR", 1.6, 0.8);   // s1 = A xor B
    S.gate("g2", "AND", 1.6, 2.0);   // c1 = A and B
    S.gate("g3", "XOR", 3.8, 0.8);   // S  = s1 xor Cin
    S.gate("g4", "AND", 3.8, 2.0);   // c2 = s1 and Cin
    S.gate("g5", "OR",  6.0, 1.4);   // Cout = c1 or c2
    S.pin("A", 0.0, 0.5, "left");
    S.pin("B", 0.0, 1.3, "left");
    S.pin("Cin", 0.0, 2.4, "left");
    S.wire("A", "g1.a");
    S.wire("B", "g1.b");
    S.wire("A", "g2.a");
    S.wire("B", "g2.b");
    S.wire("g1", "g3.a");
    S.wire("Cin", "g3.b");
    S.wire("g1", "g4.a");
    S.wire("Cin", "g4.b");
    S.wire("g2", "g5.a");
    S.wire("g4", "g5.b");
    S.pin("S", 7.8, 0.8, "right");
    S.pin("Cout", 7.8, 2.0, "right");
    S.wire("g3", "S");
    S.wire("g5", "Cout");
    return S;
}

void fullAdder(int A, int B, int Cin, const std::vector<std::vector<int>>& rows, int rowIdx) {
    Schematic sch = layout();
    int s1 = XOR(A, B), c1 = AND(A, B), S = XOR(s1, Cin), c2 = AND(s1, Cin), Cout = OR(c1, c2);
    std::map<std::string, int> v{{"A", A}, {"B", B}, {"Cin", Cin},
                                  {"g1", s1}, {"g2", c1}, {"g3", S}, {"g4", c2}, {"g5", Cout},
                                  {"S", S}, {"Cout", Cout}};
    auto rowLabels = std::vector<std::string>{"0,0,0", "0,0,1", "0,1,0", "0,1,1",
                                               "1,0,0", "1,0,1", "1,1,0", "1,1,1"};
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"Cin", Cin}, {"S", S}, {"Cout", Cout}})
     .circuitState(sch.json(v, "g1"))
     .matrixState("truth", truthTable({"A", "B", "Cin", "S", "Cout"}, rowLabels, rows, rowIdx))
     .note("A XOR B = " + std::to_string(s1) + ", A AND B = " + std::to_string(c1) + ".")
     .emit();
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"Cin", Cin}, {"S", S}, {"Cout", Cout}})
     .circuitState(sch.json(v, "g3"))
     .matrixState("truth", truthTable({"A", "B", "Cin", "S", "Cout"}, rowLabels, rows, rowIdx))
     .note("S = (A XOR B) XOR Cin = " + std::to_string(S) + ".")
     .emit();
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"Cin", Cin}, {"S", S}, {"Cout", Cout}})
     .circuitState(sch.json(v, "g5"))
     .matrixState("truth", truthTable({"A", "B", "Cin", "S", "Cout"}, rowLabels, rows, rowIdx))
     .note("Cout = (A AND B) OR (Cin AND (A XOR B)) = " + std::to_string(Cout) + ".")
     .emit();
}

int main() {
    T.view("schematic").complexity("3 t_p", "5 gate");
    T.panel("matrix", "Truth table", "truth");

    std::vector<std::vector<int>> rows;
    int combos[8][3] = {{0,0,0},{0,0,1},{0,1,0},{0,1,1},{1,0,0},{1,0,1},{1,1,0},{1,1,1}};
    for (auto& c : combos) {
        int s1 = XOR(c[0], c[1]), S = XOR(s1, c[2]);
        int Cout = OR(AND(c[0], c[1]), AND(s1, c[2]));
        rows.push_back({c[0], c[1], c[2], S, Cout});
    }

    for (int i = 0; i < 8; i++) {
        T.input("A=" + std::to_string(combos[i][0]) + " B=" + std::to_string(combos[i][1]) +
                " Cin=" + std::to_string(combos[i][2]));
        fullAdder(combos[i][0], combos[i][1], combos[i][2], rows, i);
    }

    T.dump("traces/full_adder.js");
}
