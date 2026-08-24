// A Practical Moore Machine — The Traffic Light Controller
//
// Three states, one timer per state: Red lasts longer than Yellow because it
// also has to cover the cross-traffic clearance time, Yellow is the
// shortest because it's just a warning. The output (the lit colour) depends
// only on the state, never on an external input — a pure, free-running
// Moore machine.
#include <string>
#include "../tracer/tracer.hpp"

static Tracer T("traffic_light", "Traffic Light", "FSM", __FILE__);

static const char* names[3] = {"Red", "Green", "Yellow"};
static const int durations[3] = {4, 3, 1};

static std::string graphOf(int state) {
    auto st = [&](int n) { return n == state ? std::string("current") : std::string(""); };
    return "{\"nodes\":{"
           "\"Red\":{\"state\":\"" + st(0) + "\"},"
           "\"Green\":{\"state\":\"" + st(1) + "\"},"
           "\"Yellow\":{\"state\":\"" + st(2) + "\"}"
           "},\"edges\":["
           "{\"from\":\"Red\",\"to\":\"Green\",\"label\":\"timer\",\"directed\":true,\"state\":\"" +
           (state == 1 ? "current" : "") + "\"},"
           "{\"from\":\"Green\",\"to\":\"Yellow\",\"label\":\"timer\",\"directed\":true,\"state\":\"" +
           (state == 2 ? "current" : "") + "\"},"
           "{\"from\":\"Yellow\",\"to\":\"Red\",\"label\":\"timer\",\"directed\":true,\"state\":\"" +
           (state == 0 ? "current" : "") + "\"}"
           "]}";
}

int main() {
    T.view("graph").complexity("O(1) per tick", "3 states");
    T.panel("table", "Duration per state (ticks)");

    T.input("Two full cycles");
    T.layout("{\"Red\":{\"x\":0,\"y\":0},\"Green\":{\"x\":150,\"y\":80},\"Yellow\":{\"x\":0,\"y\":160}}");

    int state = 0, tick = 0;
    for (int cycle = 0; cycle < 2; cycle++) {
        for (int s = 0; s < 3; s++) {
            state = s;
            for (int i = 0; i < durations[s]; i++) {
                T.at(__LINE__, "main")
                 .vars({{"tick", tick}, {"state", state}, {"remaining", durations[s] - i}})
                 .graphState(graphOf(state))
                 .table("durations", {{"Red", durations[0]}, {"Green", durations[1]}, {"Yellow", durations[2]}})
                 .note(std::string(names[state]) + ": tick " + std::to_string(i + 1) + " of " +
                       std::to_string(durations[s]) + ".")
                 .emit();
                tick++;
            }
        }
    }

    T.dump("traces/traffic_light.js");
}
