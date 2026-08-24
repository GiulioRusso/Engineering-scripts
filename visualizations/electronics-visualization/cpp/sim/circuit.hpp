// circuit.hpp - schematic snapshot builder, the analogue of views.hpp.
//
// A circuit declares its gates, wires and pins once, with hand-placed
// coordinates (never an auto-layout — see the algorithm visualizer's GUIDE.md
// for why). Each step then supplies the current bit value of every named
// signal and asks for a JSON snapshot; `schematic.js` reads that JSON, never
// the C++.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "../tracer/tracer.hpp"

namespace trace {

struct GateSpec { std::string id, type; double x, y; };
struct PinSpec  { std::string name; double x, y; std::string side; };
struct WireSpec { std::string from, to; };

class Schematic {
  public:
    Schematic& gate(std::string id, std::string type, double x, double y) {
        gates_.push_back({std::move(id), std::move(type), x, y});
        return *this;
    }
    // `from` must be a signal name that appears in the `values` map passed to
    // json() (a pin name or a gate id, which doubles as that gate's output
    // signal). `to` is only used to locate the wire's other end on screen and
    // may be "<gateId>.<port>" with port one of a / b / out.
    Schematic& wire(std::string from, std::string to) {
        wires_.push_back({std::move(from), std::move(to)});
        return *this;
    }
    Schematic& pin(std::string name, double x, double y, std::string side = "left") {
        pins_.push_back({std::move(name), x, y, std::move(side)});
        return *this;
    }

    // `active` names the gate id currently being evaluated, if any, so the
    // renderer can highlight it.
    std::string json(const std::map<std::string, int>& values,
                      const std::string& active = "") const {
        auto lvl = [&](const std::string& sig) -> int {
            auto it = values.find(sig);
            return it == values.end() ? 0 : it->second;
        };

        std::string s = "{\"gates\":[";
        for (size_t i = 0; i < gates_.size(); i++) {
            if (i) s += ",";
            const auto& g = gates_[i];
            s += object({{"id", g.id}, {"type", g.type}, {"x", g.x}, {"y", g.y},
                          {"out", lvl(g.id)},
                          {"state", g.id == active ? std::string("active") : std::string("")}});
        }
        s += "],\"wires\":[";
        for (size_t i = 0; i < wires_.size(); i++) {
            if (i) s += ",";
            const auto& w = wires_[i];
            s += object({{"from", w.from}, {"to", w.to}, {"bit", lvl(w.from)}});
        }
        s += "],\"pins\":[";
        for (size_t i = 0; i < pins_.size(); i++) {
            if (i) s += ",";
            const auto& p = pins_[i];
            s += object({{"name", p.name}, {"x", p.x}, {"y", p.y},
                          {"bit", lvl(p.name)}, {"side", p.side}});
        }
        return s + "]}";
    }

  private:
    std::vector<GateSpec> gates_;
    std::vector<WireSpec> wires_;
    std::vector<PinSpec> pins_;
};

}  // namespace trace

using trace::Schematic;
