// waves.hpp - waveform accumulator, the analogue of circuit.hpp.
//
// A circuit precomputes every signal's full value history once (waveforms are
// known ahead of time, exactly like the array a sort runs over), then each
// step asks for a snapshot at a given time index `t`. The renderer draws the
// whole stepped trace and moves a cursor to `t`; it never recomputes a signal.
#pragma once

#include <string>
#include <vector>

#include "../tracer/tracer.hpp"

namespace trace {

struct WaveSignal { std::string name, kind; std::vector<int> values; };

class Waves {
  public:
    // kind: "clk" | "data" | "out" - only affects styling in the renderer.
    Waves& add(std::string name, std::string kind, std::vector<int> values) {
        sigs_.push_back({std::move(name), std::move(kind), std::move(values)});
        return *this;
    }

    // `markersJson` is a raw JSON array of {t,label} objects (default: none).
    // `windowJson` is a raw JSON {from,to,label} object, or empty to omit the
    // window entirely (used for the setup/hold shaded region).
    std::string json(int t, const std::string& markersJson = "[]",
                      const std::string& windowJson = "") const {
        std::string s = "{\"t\":" + std::to_string(t) + ",\"signals\":[";
        for (size_t i = 0; i < sigs_.size(); i++) {
            if (i) s += ",";
            const auto& sg = sigs_[i];
            std::string vals = "[";
            for (size_t j = 0; j < sg.values.size(); j++) {
                if (j) vals += ",";
                vals += std::to_string(sg.values[j]);
            }
            vals += "]";
            s += "{\"name\":" + Val::quote(sg.name) + ",\"kind\":" + Val::quote(sg.kind) +
                 ",\"values\":" + vals + "}";
        }
        s += "],\"markers\":" + markersJson;
        if (!windowJson.empty()) s += ",\"window\":" + windowJson;
        return s + "}";
    }

  private:
    std::vector<WaveSignal> sigs_;
};

}  // namespace trace

using trace::Waves;
