// 1-Bit Comparator — Greater, Equal, Less
//
// GT = A AND (NOT B); LT = (NOT A) AND B; EQ = XNOR(A, B). Exactly one of the
// three is always 1: they are mutually exclusive by construction.
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/logic.hpp"
#include "../sim/circuit.hpp"
#include "../sim/truth.hpp"

static Tracer T("comparator_1bit", "1-Bit Comparator", "Adders", __FILE__);

static Schematic layout() {
    Schematic S;
    S.gate("g1", "NOT",  1.6, 0.5);   // notA
    S.gate("g2", "NOT",  1.6, 2.3);   // notB
    S.gate("g3", "AND",  3.8, 0.8);   // GT = A and notB
    S.gate("g4", "AND",  3.8, 2.0);   // LT = notA and B
    S.gate("g5", "XNOR", 3.8, 1.4);   // EQ
    S.pin("A", 0.0, 0.5, "left");
    S.pin("B", 0.0, 2.3, "left");
    S.wire("A", "g1.a");
    S.wire("B", "g2.a");
    S.wire("A", "g3.a");
    S.wire("g2", "g3.b");
    S.wire("g1", "g4.a");
    S.wire("B", "g4.b");
    S.wire("A", "g5.a");
    S.wire("B", "g5.b");
    S.pin("GT", 5.6, 0.8, "right");
    S.pin("EQ", 5.6, 1.4, "right");
    S.pin("LT", 5.6, 2.0, "right");
    S.wire("g3", "GT");
    S.wire("g5", "EQ");
    S.wire("g4", "LT");
    return S;
}

void compare(int A, int B, const std::vector<std::vector<int>>& rows, int rowIdx) {
    Schematic sch = layout();
    int notA = NOT(A), notB = NOT(B);
    int GT = AND(A, notB), LT = AND(notA, B), EQ = XNOR(A, B);
    std::map<std::string, int> v{{"A", A}, {"B", B}, {"g1", notA}, {"g2", notB},
                                  {"g3", GT}, {"g4", LT}, {"g5", EQ}, {"GT", GT}, {"EQ", EQ}, {"LT", LT}};
    auto rowLabels = std::vector<std::string>{"0,0", "0,1", "1,0", "1,1"};
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"GT", GT}, {"EQ", EQ}, {"LT", LT}})
     .circuitState(sch.json(v, "g3"))
     .matrixState("truth", truthTable({"A", "B", "GT", "EQ", "LT"}, rowLabels, rows, rowIdx))
     .note("GT = A AND (NOT B) = " + std::to_string(GT) + ".")
     .emit();
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"GT", GT}, {"EQ", EQ}, {"LT", LT}})
     .circuitState(sch.json(v, "g5"))
     .matrixState("truth", truthTable({"A", "B", "GT", "EQ", "LT"}, rowLabels, rows, rowIdx))
     .note("EQ = XNOR(A, B) = " + std::to_string(EQ) + ".")
     .emit();
    T.at(__LINE__, __func__)
     .vars({{"A", A}, {"B", B}, {"GT", GT}, {"EQ", EQ}, {"LT", LT}})
     .circuitState(sch.json(v, "g4"))
     .matrixState("truth", truthTable({"A", "B", "GT", "EQ", "LT"}, rowLabels, rows, rowIdx))
     .note("LT = (NOT A) AND B = " + std::to_string(LT) + ".")
     .emit();
}

int main() {
    T.view("schematic").complexity("2 t_p", "5 gate");
    T.panel("matrix", "Truth table", "truth");

    int combos[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    std::vector<std::vector<int>> rows;
    for (auto& c : combos) {
        int GT = AND(c[0], NOT(c[1])), LT = AND(NOT(c[0]), c[1]), EQ = XNOR(c[0], c[1]);
        rows.push_back({c[0], c[1], GT, EQ, LT});
    }

    for (int i = 0; i < 4; i++) {
        T.input("A=" + std::to_string(combos[i][0]) + " B=" + std::to_string(combos[i][1]));
        compare(combos[i][0], combos[i][1], rows, i);
    }

    T.dump("traces/comparator_1bit.js");
}
