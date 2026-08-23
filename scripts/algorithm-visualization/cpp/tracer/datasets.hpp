// datasets.hpp - the predefined inputs every array algorithm is traced on.
//
// The player is static and cannot recompile, so build.sh generates a trace per
// input instead. Sizes stay at 8-10 elements: readable on a projector.
#pragma once

#include <string>
#include <vector>

namespace trace {

struct Dataset {
    std::string label;
    std::vector<int> data;
};

// Unsorted inputs: the four classic shapes plus the already-sorted one, which
// is what makes bubble sort's early exit visible.
inline std::vector<Dataset> sortInputs() {
    return {
        {"casuale",        {5, 3, 8, 6, 2, 7, 1, 4}},
        {"quasi ordinato", {1, 2, 4, 3, 5, 6, 8, 7}},
        {"invertito (caso peggiore)", {8, 7, 6, 5, 4, 3, 2, 1}},
        {"con duplicati",  {4, 2, 4, 1, 3, 2, 4, 1}},
        {"gia ordinato",   {1, 2, 3, 4, 5, 6, 7, 8}},
    };
}

// Sorted, for binary search.
inline std::vector<int> sortedData() {
    return {2, 5, 8, 12, 16, 23, 38, 56, 72, 91};
}

// Unsorted, for linear search.
inline std::vector<int> unsortedData() {
    return {23, 5, 72, 8, 91, 16, 2, 38, 56, 12};
}

}  // namespace trace

using trace::Dataset;
