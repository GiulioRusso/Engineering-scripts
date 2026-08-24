// Kadane
//
// Finds the maximum sum of a contiguous subarray in a single pass. The
// insight: a prefix with a negative sum can never help the subarray that
// follows it, so it gets thrown away and the search restarts from zero.
#include <climits>
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("kadane", "Kadane", "Other", __FILE__);

int maxSubarraySum(int a[], int n) {
    int maxSum = INT_MIN;
    int currentSum = 0;
    int start = 0, bestStart = 0, bestEnd = 0;

    for (int i = 0; i < n; i++) {
        currentSum = currentSum + a[i];
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"a[i]", a[i]}, {"currentSum", currentSum}, {"maxSum", maxSum == INT_MIN ? std::string("-inf") : std::to_string(maxSum)}})
         .marks({{"i", i}})
         .region("window", start, i)
         .region("sorted", bestStart, bestEnd)
         .select(i)
         .counters({{"steps", i + 1}})
         .note("Extending the current subarray with a[" + std::to_string(i) + "] = " +
               std::to_string(a[i]) + ": currentSum = " + std::to_string(currentSum) + ".")
         .emit();

        if (currentSum > maxSum) {
            maxSum = currentSum;
            bestStart = start;
            bestEnd = i;
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"currentSum", currentSum}, {"maxSum", maxSum}})
             .marks({{"i", i}})
             .region("window", start, i)
             .region("sorted", bestStart, bestEnd)
             .found(i)
             .note("New maximum: maxSum = " + std::to_string(maxSum) + ", reached by the subarray [" +
                   std::to_string(bestStart) + ", " + std::to_string(bestEnd) + "].")
             .emit();
        }

        if (currentSum < 0) {
            currentSum = 0;
            start = i + 1;
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"currentSum", currentSum}, {"maxSum", maxSum}, {"start", start}})
             .marks({{"i", i}})
             .region("excluded", 0, i)
             .region("sorted", bestStart, bestEnd)
             .discard(0, i)
             .note("currentSum is negative: this prefix can only hurt what follows. "
                   "Resetting to zero and restarting at a[" + std::to_string(start) + "]. This is Kadane's key event.")
             .emit();
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"maxSum", maxSum}})
     .region("sorted", bestStart, bestEnd)
     .note("Maximum sum " + std::to_string(maxSum) + " over the subarray [" + std::to_string(bestStart) +
           ", " + std::to_string(bestEnd) + "], found in a single pass: O(n) time, O(1) space.")
     .emit();
    return maxSum;
}

int main() {
    T.view("array").complexity("O(n)", "O(1)");

    const std::vector<Dataset> cases = {
        {"classic example", {-2, 1, -3, 4, -1, 2, 1, -5, 4}},
        {"all negative", {-3, -1, -7, -2, -5}},
        {"all positive", {2, 4, 1, 3, 5, 2}},
        {"maximum straddling a negative", {5, -2, 7, -8, 3, 4}},
    };

    for (const Dataset& ds : cases) {
        int n = static_cast<int>(ds.data.size());
        int a[16];
        for (int i = 0; i < n; i++) a[i] = ds.data[i];

        T.input(ds.label);
        T.clearWatches().watchArray("a", a, n);
        T.at(__LINE__, "maxSubarraySum")
         .vars({{"n", n}, {"currentSum", 0}, {"maxSum", "-inf"}})
         .counters({{"steps", 0}})
         .note("Starting array. currentSum and maxSum are the only two values to keep an eye on.")
         .emit();

        maxSubarraySum(a, n);
    }

    T.dump("traces/kadane.js");
    return 0;
}
