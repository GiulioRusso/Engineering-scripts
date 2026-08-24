// Bubble Sort
//
// Compares adjacent pairs and swaps them if they're out of order. Each pass
// the largest element "bubbles up" to the end, so the sorted right edge
// grows. The swapped flag allows an early exit: on an already-sorted array a
// single pass is enough, i.e. O(n).
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("bubble_sort", "Bubble Sort", "Sorting", __FILE__);
static int comparisons = 0, swaps = 0;

void bubbleSort(int a[], int n) {
    for (int i = 0; i < n - 1; i++) {
        bool swapped = false;
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"swapped", swapped}})
         .region("sorted", n - i, n - 1)
         .counters({{"comparisons", comparisons}, {"swaps", swaps}})
         .note("Pass " + std::to_string(i + 1) + ": swapped starts back at false.")
         .emit();

        for (int j = 0; j < n - i - 1; j++) {
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"j", j}, {"a[j]", a[j]}, {"a[j+1]", a[j + 1]}, {"swapped", swapped}})
             .marks({{"j", j}})
             .region("sorted", n - i, n - 1)
             .region("window", j, j + 1)
             .compare(j, j + 1)
             .counters({{"comparisons", ++comparisons}, {"swaps", swaps}})
             .note("Comparing the adjacent pair a[" + std::to_string(j) + "] = " + std::to_string(a[j]) +
                   " and a[" + std::to_string(j + 1) + "] = " + std::to_string(a[j + 1]) + ".")
             .emit();

            if (a[j] > a[j + 1]) {
                T.at(__LINE__, __func__)
                 .vars({{"i", i}, {"j", j}, {"swapped", true}})
                 .marks({{"j", j}})
                 .region("sorted", n - i, n - 1)
                 .swap(j, j + 1)
                 .counters({{"comparisons", comparisons}, {"swaps", ++swaps}})
                 .note("Out of order: swapping the two elements and raising swapped.")
                 .emit();

                int temp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = temp;
                swapped = true;
            }
        }

        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"swapped", swapped}})
         .region("sorted", n - i - 1, n - 1)
         .counters({{"comparisons", comparisons}, {"swaps", swaps}})
         .note(std::string("End of pass: a[") + std::to_string(n - i - 1) +
               "] is now in its final place.")
         .emit();

        if (!swapped) {
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"swapped", swapped}})
             .region("sorted", 0, n - 1)
             .counters({{"comparisons", comparisons}, {"swaps", swaps}})
             .note("No swap in the whole pass: the array is already sorted, exiting early.")
             .emit();
            break;
        }
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
        T.at(__LINE__, "bubbleSort")
         .counters({{"comparisons", 0}, {"swaps", 0}})
         .note("Starting array. Watch the right edge: it grows by one cell per pass.")
         .emit();

        bubbleSort(a, n);

        T.at(__LINE__, "bubbleSort")
         .region("sorted", 0, n - 1)
         .counters({{"comparisons", comparisons}, {"swaps", swaps}})
         .note("Sorted with " + std::to_string(comparisons) + " comparisons and " + std::to_string(swaps) +
               " swaps.")
         .emit();
    }

    T.dump("traces/bubble_sort.js");
    return 0;
}
