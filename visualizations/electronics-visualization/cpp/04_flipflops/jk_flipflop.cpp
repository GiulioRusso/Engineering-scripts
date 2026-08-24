// JK Flip-Flop — Internal Gates and the Toggle Feedback
//
// next(Q) = J AND (NOT Q), OR (NOT K) AND Q. Unlike SR, J=K=1 is not
// forbidden: it resolves to TOGGLE, Q flips. It's exactly this feedback — the
// new Q depends on the old Q — that lets a JK act as a counter, something an
// SR can't do.
#include <map>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/circuit.hpp"
#include "../sim/truth.hpp"

static Tracer T("jk_flipflop", "JK Flip-Flop", "Flip-Flops", __FILE__);

static Schematic layout() {
    Schematic S;
    S.gate("master", "MS", 2.0, 1.4);
    S.gate("slave", "MS", 4.4, 1.4);
    S.pin("J", 0.0, 0.6, "left");
    S.pin("K", 0.0, 1.4, "left");
    S.pin("CLK", 0.0, 2.4, "left");
    S.wire("J", "master.a");
    S.wire("K", "master.b");
    S.wire("master", "slave.a");
    S.pin("Q", 6.2, 1.4, "right");
    S.wire("slave", "Q");
    return S;
}

static int jkNext(int J, int K, int Q) { return (J & (1 - Q)) | ((1 - K) & Q); }

int main() {
    T.view("schematic").complexity("edge-triggered", "2 latches");
    T.panel("matrix", "next(Q) = f(J, K, Q)");

    std::vector<std::vector<int>> rows;
    int combos[8][3] = {{0,0,0},{0,0,1},{0,1,0},{0,1,1},{1,0,0},{1,0,1},{1,1,0},{1,1,1}};
    for (auto& c : combos) rows.push_back({c[0], c[1], c[2], jkNext(c[0], c[1], c[2])});

    Schematic sch = layout();
    int Q = 0, masterQ = 0;

    std::vector<int> Jv = {1, 1, 0, 0, 1, 1, 1, 1};
    std::vector<int> Kv = {0, 0, 0, 0, 1, 1, 1, 1};
    std::vector<int> Cv = {0, 1, 0, 1, 0, 1, 0, 1};

    T.input("Set, hold, then repeated toggle (J=K=1)");
    for (size_t t = 0; t < Cv.size(); t++) {
        int J = Jv[t], K = Kv[t], CLK = Cv[t];
        std::string active;
        int rowIdx = J * 4 + K * 2 + Q;
        if (CLK == 0) {
            masterQ = jkNext(J, K, Q);
            active = "master";
        } else {
            Q = masterQ;
            active = "slave";
        }
        std::map<std::string, int> v{{"J", J}, {"K", K}, {"CLK", CLK}, {"master", masterQ}, {"slave", Q}, {"Q", Q}};
        T.at(__LINE__, __func__)
         .vars({{"J", J}, {"K", K}, {"CLK", CLK}, {"masterQ", masterQ}, {"Q", Q}})
         .circuitState(sch.json(v, active))
         .matrixState("truth", truthTable({"J", "K", "Q", "next"},
                                           {"0,0,0","0,0,1","0,1,0","0,1,1","1,0,0","1,0,1","1,1,0","1,1,1"},
                                           rows, rowIdx))
         .note(CLK == 0
               ? "CLK=0: master computes next(Q) = " + std::to_string(masterQ) + "."
               : "CLK=1: slave copies -> Q = " + std::to_string(Q) +
                 (J && K ? " (J=K=1: toggle)." : "."))
         .emit();
    }

    T.dump("traces/jk_flipflop.js");
}
