// The Tri-State Buffer — A Third State
//
// EN=1: OUT follows D. EN=0: OUT is neither 0 nor 1, it's disconnected — high
// impedance (Z). This is what lets multiple drivers share the same bus
// without short-circuiting, as long as at most one is enabled at a time.
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/circuit.hpp"
#include "../sim/waves.hpp"

static Tracer T("tri_state", "Tri-State Buffer", "Logic Gates", __FILE__);

static const int N = 16, PD = 3, PEN = 6;

static Schematic layout() {
    Schematic S;
    S.gate("g1", "TRI", 2.2, 1.4);
    S.pin("D", 0.0, 1.0, "left");
    S.pin("EN", 0.0, 1.8, "left");
    S.wire("D", "g1.a");
    S.wire("EN", "g1.b");
    S.pin("OUT", 4.0, 1.4, "right");
    S.wire("g1", "OUT");
    return S;
}

int main() {
    T.view("schematic").complexity("1 t_p", "1 buffer");
    T.panel("waveform", "D, EN, OUT over time");

    std::vector<int> D(N), EN(N), OUT(N);
    for (int t = 0; t < N; t++) {
        D[t] = (t / PD) % 2;
        EN[t] = (t / PEN) % 2;
        OUT[t] = EN[t] ? D[t] : -1;
    }
    Waves W;
    W.add("D", "data", D);
    W.add("EN", "data", EN);
    W.add("OUT", "out", OUT);

    Schematic sch = layout();
    T.input("D and EN independent");
    for (int t = 0; t < N; t++) {
        std::map<std::string, int> v{{"D", D[t]}, {"EN", EN[t]}, {"g1", OUT[t]}, {"OUT", OUT[t]}};
        T.at(__LINE__, __func__)
         .vars({{"t", t}, {"D", D[t]}, {"EN", EN[t]}, {"OUT", OUT[t]}})
         .circuitState(sch.json(v, "g1"))
         .waveState(W.json(t))
         .note(EN[t]
               ? "EN=1: the buffer is enabled, OUT follows D = " + std::to_string(D[t]) + "."
               : "EN=0: the buffer is disabled, OUT is high impedance (Z), neither 0 nor 1.")
         .emit();
    }

    T.dump("traces/tri_state.js");
}
