// Sequence Detector '101' (Moore) — Diagram and a Live Run
//
// Four states: S0 (no progress), S1 (seen "1"), S2 (seen "10"), S3 (seen
// "101", output 1). Detection is overlapping: from S3, another 1 goes
// straight to S1 (that 1 might open the next occurrence) and a 0 goes to S2,
// not S0 — the 1 that just closed the "101" match is also the initial 1 of a
// possible brand-new "10", and losing it would be a bug.
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../sim/waves.hpp"

static Tracer T("seq_detector_101", "'101' Sequence Detector (Moore)", "FSM", __FILE__);

static int nextState(int s, int bit) {
    switch (s) {
        case 0: return bit ? 1 : 0;
        case 1: return bit ? 1 : 2;
        case 2: return bit ? 3 : 0;
        // S3 on 0 is NOT a hard reset to S0: the '1' that just completed the
        // match is simultaneously the '1' that could start the NEXT one, and
        // this '0' extends it to "10" - a real partial match, state S2. Only
        // a genuine mismatch (nothing at all survives) goes to S0.
        case 3: return bit ? 1 : 2;
    }
    return 0;
}

static std::string graphOf(int state, int prevState) {
    // S3 gets its own colour (found) whenever it is the current state; every
    // other state just gets the usual "current" highlight.
    auto st = [&](int n) {
        if (n != state) return std::string("");
        return n == 3 ? std::string("found") : std::string("current");
    };
    std::string cur = "S" + std::to_string(prevState) + "-S" + std::to_string(state);
    auto es = [&](int from, int to) {
        return ("S" + std::to_string(from) + "-S" + std::to_string(to)) == cur ? "current" : "";
    };
    return "{\"nodes\":{"
           "\"S0\":{\"state\":\"" + st(0) + "\"},"
           "\"S1\":{\"state\":\"" + st(1) + "\"},"
           "\"S2\":{\"state\":\"" + st(2) + "\"},"
           "\"S3\":{\"state\":\"" + st(3) + "\",\"label\":\"out=1\"}"
           "},\"edges\":["
           "{\"from\":\"S0\",\"to\":\"S0\",\"label\":\"0\",\"directed\":true,\"state\":\"" + std::string(es(0,0)) + "\"},"
           "{\"from\":\"S0\",\"to\":\"S1\",\"label\":\"1\",\"directed\":true,\"state\":\"" + std::string(es(0,1)) + "\"},"
           "{\"from\":\"S1\",\"to\":\"S2\",\"label\":\"0\",\"directed\":true,\"state\":\"" + std::string(es(1,2)) + "\"},"
           "{\"from\":\"S1\",\"to\":\"S1\",\"label\":\"1\",\"directed\":true,\"state\":\"" + std::string(es(1,1)) + "\"},"
           "{\"from\":\"S2\",\"to\":\"S0\",\"label\":\"0\",\"directed\":true,\"state\":\"" + std::string(es(2,0)) + "\"},"
           "{\"from\":\"S2\",\"to\":\"S3\",\"label\":\"1\",\"directed\":true,\"state\":\"" + std::string(es(2,3)) + "\"},"
           "{\"from\":\"S3\",\"to\":\"S2\",\"label\":\"0\",\"directed\":true,\"state\":\"" + std::string(es(3,2)) + "\"},"
           "{\"from\":\"S3\",\"to\":\"S1\",\"label\":\"1\",\"directed\":true,\"state\":\"" + std::string(es(3,1)) + "\"}"
           "]}";
}

int main() {
    T.view("graph").complexity("O(n)", "4 states");
    T.panel("waveform", "Input and output");
    T.panel("table", "Transition table");

    std::string bits = "0010110101";
    int N = static_cast<int>(bits.size()) + 1;

    T.input("Input: " + bits);
    T.layout("{\"S0\":{\"x\":0,\"y\":0},\"S1\":{\"x\":150,\"y\":0},"
             "\"S2\":{\"x\":150,\"y\":120},\"S3\":{\"x\":0,\"y\":120}}");

    std::vector<int> inBits(N, 0), outBits(N, 0);
    int state = 0;
    for (int i = 0; i < static_cast<int>(bits.size()); i++) {
        int bit = bits[i] - '0';
        state = nextState(state, bit);
        inBits[i + 1] = bit;
        outBits[i + 1] = state == 3 ? 1 : 0;
    }

    state = 0;
    T.at(__LINE__, "main")
     .vars({{"state", state}})
     .graphState(graphOf(state, state))
     .table("transitions", {{"S0,0", 0}, {"S0,1", 1}, {"S1,0", 2}, {"S1,1", 1},
                             {"S2,0", 0}, {"S2,1", 3}, {"S3,0", 2}, {"S3,1", 1}})
     .waveState(Waves().add("in", "data", inBits).add("out", "out", outBits).json(0))
     .note("Initial state S0.")
     .emit();

    for (int i = 0; i < static_cast<int>(bits.size()); i++) {
        int bit = bits[i] - '0';
        int prev = state;
        state = nextState(state, bit);
        Waves W;
        W.add("in", "data", inBits);
        W.add("out", "out", outBits);
        T.at(__LINE__, "main")
         .vars({{"bit", bit}, {"prevState", prev}, {"state", state}, {"out", outBits[i + 1]}})
         .graphState(graphOf(state, prev))
         .table("transitions", {{"S0,0", 0}, {"S0,1", 1}, {"S1,0", 2}, {"S1,1", 1},
                                 {"S2,0", 0}, {"S2,1", 3}, {"S3,0", 2}, {"S3,1", 1}})
         .waveState(W.json(i + 1))
         .note(state == 3 ? "S" + std::to_string(prev) + " -" + std::to_string(bit) +
                             "-> S3: '101' detected, output 1."
                           : "S" + std::to_string(prev) + " -" + std::to_string(bit) +
                             "-> S" + std::to_string(state) + ".")
         .emit();
    }

    T.dump("traces/seq_detector_101.js");
}
