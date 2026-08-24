// Logic Gates Over Time — Same Truth Table, Now as Waveforms
//
// A gate does not care whether its inputs are static or toggling: the output
// waveform is just the truth table applied instant by instant. Watching AND
// against OR on the same A/B waveforms is what makes "logical AND" and
// "electrical AND gate" the same idea.
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/logic.hpp"
#include "../sim/waves.hpp"

static Tracer T("gates_timing", "Gates Over Time", "Logic Gates", __FILE__);

static const int N = 16, PA = 3, PB = 5;

void runTwoInputGate(const std::string& name, int (*fn)(int, int)) {
    std::vector<int> A(N), B(N), OUT(N);
    for (int t = 0; t < N; t++) {
        A[t] = (t / PA) % 2;
        B[t] = (t / PB) % 2;
        OUT[t] = fn(A[t], B[t]);
    }
    Waves W;
    W.add("A", "data", A);
    W.add("B", "data", B);
    W.add(name, "out", OUT);
    for (int t = 0; t < N; t++) {
        T.at(__LINE__, __func__)
         .vars({{"t", t}, {"A", A[t]}, {"B", B[t]}, {"OUT", OUT[t]}})
         .waveState(W.json(t))
         .note("t=" + std::to_string(t) + ": " + name + "(" + std::to_string(A[t]) + ", " +
               std::to_string(B[t]) + ") = " + std::to_string(OUT[t]) + ".")
         .emit();
    }
}

void runNot() {
    std::vector<int> A(N), OUT(N);
    for (int t = 0; t < N; t++) {
        A[t] = (t / PA) % 2;
        OUT[t] = NOT(A[t]);
    }
    Waves W;
    W.add("A", "data", A);
    W.add("NOT", "out", OUT);
    for (int t = 0; t < N; t++) {
        T.at(__LINE__, __func__)
         .vars({{"t", t}, {"A", A[t]}, {"OUT", OUT[t]}})
         .waveState(W.json(t))
         .note("t=" + std::to_string(t) + ": NOT(" + std::to_string(A[t]) + ") = " +
               std::to_string(OUT[t]) + ".")
         .emit();
    }
}

int main() {
    T.view("waveform").complexity("1 t_p", "1 gate");

    struct { const char* name; int (*fn)(int, int); } gates[] = {
        {"AND", AND}, {"OR", OR}, {"NAND", NAND}, {"NOR", NOR}, {"XOR", XOR}, {"XNOR", XNOR},
    };
    for (auto& g : gates) {
        T.input(g.name);
        runTwoInputGate(g.name, g.fn);
    }
    T.input("NOT");
    runNot();

    T.dump("traces/gates_timing.js");
}
