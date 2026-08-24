// Selection Sort
//
// Each pass looks for the minimum of the unsorted part and brings it to the
// boundary. Comparisons are always n(n-1)/2, whatever the input; what changes
// is only the number of swaps, at most one per pass.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("selection_sort", "Selection Sort", "Sorting", __FILE__);
static int comparisons = 0, swaps = 0;

void selectionSort(int a[], int n) {
    for (int i = 0; i < n - 1; i++) {
        int minIndex = i;
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"minIndex", minIndex}, {"a[minIndex]", a[minIndex]}})
         .marks({{"i", i}, {"minIndex", minIndex}})
         .region("sorted", 0, i - 1)
         .select(minIndex)
         .counters({{"comparisons", comparisons}, {"swaps", swaps}})
         .note("New pass. Assuming the minimum of the unsorted part is a[" +
               std::to_string(i) + "] = " + std::to_string(a[i]) + ".")
         .emit();

        for (int j = i + 1; j < n; j++) {
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"j", j}, {"minIndex", minIndex}, {"a[j]", a[j]}, {"a[minIndex]", a[minIndex]}})
             .marks({{"i", i}, {"j", j}, {"minIndex", minIndex}})
             .region("sorted", 0, i - 1)
             .compare(j, minIndex)
             .counters({{"comparisons", ++comparisons}, {"swaps", swaps}})
             .note("Comparing a[" + std::to_string(j) + "] = " + std::to_string(a[j]) +
                   " with the current minimum a[" + std::to_string(minIndex) + "] = " +
                   std::to_string(a[minIndex]) + ".")
             .emit();

            if (a[j] < a[minIndex]) {
                minIndex = j;
                T.at(__LINE__, __func__)
                 .vars({{"i", i}, {"j", j}, {"minIndex", minIndex}, {"a[minIndex]", a[minIndex]}})
                 .marks({{"i", i}, {"j", j}, {"minIndex", minIndex}})
                 .region("sorted", 0, i - 1)
                 .select(minIndex)
                 .counters({{"comparisons", comparisons}, {"swaps", swaps}})
                 .note("Smaller than the current minimum: minIndex moves to " + std::to_string(j) + ".")
                 .emit();
            }
        }

        if (minIndex != i) {
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"minIndex", minIndex}})
             .marks({{"i", i}, {"minIndex", minIndex}})
             .region("sorted", 0, i - 1)
             .swap(i, minIndex)
             .counters({{"comparisons", comparisons}, {"swaps", ++swaps}})
             .note("Swapping a[" + std::to_string(i) + "] with the minimum found a[" +
                   std::to_string(minIndex) + "]: this is the only swap of this pass.")
             .emit();

            int temp = a[i];
            a[i] = a[minIndex];
            a[minIndex] = temp;
        }

        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"a[i]", a[i]}})
         .marks({{"i", i}})
         .region("sorted", 0, i)
         .counters({{"comparisons", comparisons}, {"swaps", swaps}})
         .note("Position " + std::to_string(i) + " is now final: the sorted part grows by one element.")
         .emit();
    }
}

int main() {
    T.view("array").complexity("O(n^2)", "O(1)");

    for (const Dataset& ds : trace::sortInputs()) {
        int n = static_cast<int>(ds.data.size());
        int a[16];
        for (int i = 0; i < n; i++) a[i] = ds.data[i];

        comparisons = swaps = 0;
        T.input(ds.label);
        T.clearWatches().watchArray("a", a, n);
        T.at(__LINE__, "selectionSort")
         .marks({{"i", 0}})
         .counters({{"comparisons", 0}, {"swaps", 0}})
         .note("Starting array, entirely unsorted.")
         .emit();

        selectionSort(a, n);

        T.at(__LINE__, "selectionSort")
         .region("sorted", 0, n - 1)
         .counters({{"comparisons", comparisons}, {"swaps", swaps}})
         .note("Array sorted with " + std::to_string(comparisons) + " comparisons and " +
               std::to_string(swaps) + " swaps. The comparisons don't depend on the input, the swaps do.")
         .emit();
    }

    T.dump("traces/selection_sort.js");
    return 0;
}
