// truth.hpp - a truth-table snapshot for the matrix renderer, used as an
// auxiliary panel next to a schematic. Reused by every entry that shows a
// truth table (gates_truth, full_adder, comparator_1bit).
#pragma once

#include <string>
#include <vector>

#include "../tracer/tracer.hpp"

namespace trace {

inline std::string truthTable(const std::vector<std::string>& cols,
                               const std::vector<std::string>& rowLabels,
                               const std::vector<std::vector<int>>& rows,
                               int hlRow) {
    std::string s = "{\"labels\":[";
    for (size_t i = 0; i < cols.size(); i++) { if (i) s += ","; s += Val::quote(cols[i]); }
    s += "],\"rowLabels\":[";
    for (size_t i = 0; i < rowLabels.size(); i++) { if (i) s += ","; s += Val::quote(rowLabels[i]); }
    s += "],\"rows\":[";
    for (size_t r = 0; r < rows.size(); r++) {
        if (r) s += ",";
        s += "[";
        for (size_t c = 0; c < rows[r].size(); c++) { if (c) s += ","; s += std::to_string(rows[r][c]); }
        s += "]";
    }
    s += "],\"hlRow\":" + std::to_string(hlRow) + "}";
    return s;
}

}  // namespace trace

using trace::truthTable;
