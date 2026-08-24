// Moore vs Mealy — Where the Output Comes From
//
// Same pair of states, same transition table: the only difference is where
// the output lives. In Moore it's attached to the STATE (a label on the
// node), so it depends only on where you are. In Mealy it's attached to the
// TRANSITION (a label on the edge), and also depends on the input that
// caused it — which is why a Mealy machine can react one cycle sooner.
#include <string>
#include "../tracer/tracer.hpp"

static Tracer T("moore_vs_mealy", "Moore vs Mealy", "FSM", __FILE__);

static std::string mooreGraph(int state, const std::string& edgeState) {
    std::string s0state = state == 0 ? "current" : "";
    std::string s1state = state == 1 ? "current" : "";
    return "{\"nodes\":{"
           "\"S0\":{\"state\":\"" + s0state + "\",\"label\":\"out=0\"},"
           "\"S1\":{\"state\":\"" + s1state + "\",\"label\":\"out=1\"}"
           "},\"edges\":["
           "{\"from\":\"S0\",\"to\":\"S0\",\"label\":\"0\",\"directed\":true,\"state\":\"" +
           (edgeState == "S0-S0" ? "current" : "") + "\"},"
           "{\"from\":\"S0\",\"to\":\"S1\",\"label\":\"1\",\"directed\":true,\"state\":\"" +
           (edgeState == "S0-S1" ? "current" : "") + "\"},"
           "{\"from\":\"S1\",\"to\":\"S0\",\"label\":\"0\",\"directed\":true,\"state\":\"" +
           (edgeState == "S1-S0" ? "current" : "") + "\"},"
           "{\"from\":\"S1\",\"to\":\"S1\",\"label\":\"1\",\"directed\":true,\"state\":\"" +
           (edgeState == "S1-S1" ? "current" : "") + "\"}"
           "]}";
}

static std::string mealyGraph(const std::string& edgeState) {
    return "{\"nodes\":{"
           "\"S0\":{\"state\":\"" + std::string(edgeState.substr(0, 2) == "S0" ? "current" : "") + "\"},"
           "\"S1\":{\"state\":\"" + std::string(edgeState.substr(0, 2) == "S1" ? "current" : "") + "\"}"
           "},\"edges\":["
           "{\"from\":\"S0\",\"to\":\"S0\",\"label\":\"0/0\",\"directed\":true,\"state\":\"" +
           (edgeState == "S0-S0" ? "current" : "") + "\"},"
           "{\"from\":\"S0\",\"to\":\"S1\",\"label\":\"1/1\",\"directed\":true,\"state\":\"" +
           (edgeState == "S0-S1" ? "current" : "") + "\"},"
           "{\"from\":\"S1\",\"to\":\"S0\",\"label\":\"0/0\",\"directed\":true,\"state\":\"" +
           (edgeState == "S1-S0" ? "current" : "") + "\"},"
           "{\"from\":\"S1\",\"to\":\"S1\",\"label\":\"1/1\",\"directed\":true,\"state\":\"" +
           (edgeState == "S1-S1" ? "current" : "") + "\"}"
           "]}";
}

int main() {
    T.view("graph").complexity("O(1) per symbol", "2 states");
    T.panel("graph", "Mealy (output on the edge)", "mealy");

    std::string bits = "1101001";
    int state = 0;
    T.input("Input: " + bits);
    T.layout("{\"S0\":{\"x\":0,\"y\":0},\"S1\":{\"x\":140,\"y\":0}}");
    T.at(__LINE__, "main")
     .vars({{"state", state}})
     .graphState(mooreGraph(state, ""))
     .graphState("mealy", mealyGraph(""))
     .note("Initial state S0, Moore output = 0.")
     .emit();

    for (char c : bits) {
        int bit = c - '0';
        int next = bit;   // this toy machine's next state is just the input bit
        std::string edgeState = "S" + std::to_string(state) + "-S" + std::to_string(next);
        state = next;
        T.at(__LINE__, "main")
         .vars({{"input", bit}, {"state", state}, {"out_moore", state}, {"out_mealy", bit}})
         .graphState(mooreGraph(state, edgeState))
         .graphState("mealy", mealyGraph(edgeState))
         .note("Input " + std::to_string(bit) + ": new state S" + std::to_string(state) +
               ". Moore output = " + std::to_string(state) + " (from the state). Mealy output = " +
               std::to_string(bit) + " (from the transition, same cycle).")
         .emit();
    }

    T.dump("traces/moore_vs_mealy.js");
}
