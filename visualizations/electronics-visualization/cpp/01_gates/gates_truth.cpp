// The Seven Gates — From the Schematic to the Truth Table
//
// Each of the book's seven gates, one at a time: the same schematic, driven
// through every input combination, with the truth table lighting up the row
// that matches what is on screen right now.
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/logic.hpp"
#include "../sim/circuit.hpp"
#include "../sim/truth.hpp"

static Tracer T("gates_truth", "The Seven Gates", "Logic Gates", __FILE__);

static Schematic gateLayout(const std::string& type, bool twoInput) {
    Schematic S;
    S.gate("g1", type, 2.2, twoInput ? 1.6 : 1.0);
    S.pin("A", 0.0, twoInput ? 1.2 : 1.0, "left");
    S.wire("A", "g1.a");
    if (twoInput) {
        S.pin("B", 0.0, 2.0, "left");
        S.wire("B", "g1.b");
    }
    S.pin("OUT", 4.0, twoInput ? 1.6 : 1.0, "right");
    S.wire("g1", "OUT");
    return S;
}

void runTwoInputGate(const std::string& name, int (*fn)(int, int)) {
    Schematic sch = gateLayout(name, true);
    int combos[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    std::vector<std::vector<int>> rows;
    for (auto& c : combos) rows.push_back({c[0], c[1], fn(c[0], c[1])});

    for (int i = 0; i < 4; i++) {
        int a = combos[i][0], b = combos[i][1], out = fn(a, b);
        std::map<std::string, int> v{{"A", a}, {"B", b}, {"g1", out}, {"OUT", out}};
        T.at(__LINE__, __func__)
         .vars({{"A", a}, {"B", b}, {"OUT", out}})
         .circuitState(sch.json(v, "g1"))
         .matrixState("truth", truthTable({"A", "B", "OUT"}, {"0,0", "0,1", "1,0", "1,1"}, rows, i))
         .note(name + "(" + std::to_string(a) + ", " + std::to_string(b) + ") = " + std::to_string(out) + ".")
         .emit();
    }
}

void runNot() {
    Schematic sch = gateLayout("NOT", false);
    std::vector<std::vector<int>> rows = {{0, NOT(0)}, {1, NOT(1)}};

    for (int i = 0; i < 2; i++) {
        int a = i, out = NOT(a);
        std::map<std::string, int> v{{"A", a}, {"g1", out}, {"OUT", out}};
        T.at(__LINE__, __func__)
         .vars({{"A", a}, {"OUT", out}})
         .circuitState(sch.json(v, "g1"))
         .matrixState("truth", truthTable({"A", "OUT"}, {"0", "1"}, rows, i))
         .note("NOT(" + std::to_string(a) + ") = " + std::to_string(out) + ".")
         .emit();
    }
}

int main() {
    T.view("schematic").complexity("1 t_p", "1 gate");
    T.panel("matrix", "Truth table", "truth");

    struct { const char* name; int (*fn)(int, int); } gates[] = {
        {"AND", AND}, {"OR", OR}, {"NAND", NAND}, {"NOR", NOR}, {"XOR", XOR}, {"XNOR", XNOR},
    };
    for (auto& g : gates) {
        T.input(g.name);
        runTwoInputGate(g.name, g.fn);
    }
    T.input("NOT");
    runNot();

    T.dump("traces/gates_truth.js");
}
