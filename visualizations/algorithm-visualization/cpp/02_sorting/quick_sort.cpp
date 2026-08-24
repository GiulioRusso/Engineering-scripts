// Quick Sort
//
// Picks a pivot (here the last element), partitions the array into "less than
// the pivot" and "not less", then recursively sorts the two parts. The pivot
// ends up in its final position and never moves again.
//
// On already-sorted input the pivot is always the maximum: one partition
// stays empty, the recursion becomes n deep, and the cost degenerates to
// O(n²).
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("quick_sort", "Quick Sort", "Sorting", __FILE__);
static int comparisons = 0, swaps = 0, maxDepth = 0;

static std::string range(int l, int r) { return "[" + std::to_string(l) + ", " + std::to_string(r) + "]"; }

int partition(int a[], int low, int high) {
    int pivot = a[high];
    int i = low - 1;
    T.at(__LINE__, __func__)
     .vars({{"low", low}, {"high", high}, {"pivot", pivot}, {"i", i}})
     .marks({{"high", high}, {"i", i}})
     .region("window", low, high)
     .select(high)
     .counters({{"comparisons", comparisons}, {"swaps", swaps}, {"depth", maxDepth}})
     .note("Pivot = a[" + std::to_string(high) + "] = " + std::to_string(pivot) +
           ". i starts at low-1: it's the boundary of the less-than region, empty for now.")
     .emit();

    for (int j = low; j < high; j++) {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"j", j}, {"pivot", pivot}, {"a[j]", a[j]}})
         .marks({{"i", i}, {"j", j}, {"high", high}})
         .region("window", low, high)
         .region("sorted", low, i)
         .compare(j, high)
         .counters({{"comparisons", ++comparisons}, {"swaps", swaps}, {"depth", maxDepth}})
         .note("a[" + std::to_string(j) + "] = " + std::to_string(a[j]) + " against the pivot " +
               std::to_string(pivot) + ".")
         .emit();

        if (a[j] < pivot) {
            i++;
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"j", j}, {"pivot", pivot}})
             .marks({{"i", i}, {"j", j}, {"high", high}})
             .region("window", low, high)
             .region("sorted", low, i - 1)
             .swap(i, j)
             .counters({{"comparisons", comparisons}, {"swaps", ++swaps}, {"depth", maxDepth}})
             .note("Less than the pivot: widening the less-than region to i = " + std::to_string(i) +
                   " and bringing a[" + std::to_string(j) + "] into it.")
             .emit();

            int temp = a[i];
            a[i] = a[j];
            a[j] = temp;
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"i", i}, {"high", high}, {"pivot", pivot}})
     .marks({{"i", i}, {"high", high}})
     .region("window", low, high)
     .region("sorted", low, i)
     .swap(i + 1, high)
     .counters({{"comparisons", comparisons}, {"swaps", ++swaps}, {"depth", maxDepth}})
     .note("The decisive moment: the pivot jumps over the less-than region and lands in a[" +
           std::to_string(i + 1) + "]. From here on it will never move again.")
     .emit();

    int temp = a[i + 1];
    a[i + 1] = a[high];
    a[high] = temp;

    T.at(__LINE__, __func__)
     .vars({{"pivotIndex", i + 1}})
     .marks({{"pivot", i + 1}})
     .region("sorted", i + 1, i + 1)
     .found(i + 1)
     .counters({{"comparisons", comparisons}, {"swaps", swaps}, {"depth", maxDepth}})
     .note("Pivot's final position: " + std::to_string(i + 1) + ". Smaller ones on the left, larger ones on the right.")
     .emit();
    return i + 1;
}

void quickSort(int a[], int low, int high) {
    T.pushFrame("quickSort(" + std::to_string(low) + ", " + std::to_string(high) + ")");
    if (T.depth() > maxDepth) maxDepth = T.depth();

    if (low < high) {
        int pivotIndex = partition(a, low, high);

        T.at(__LINE__, __func__)
         .vars({{"low", low}, {"high", high}, {"pivotIndex", pivotIndex}})
         .region("window", low, high)
         .region("sorted", pivotIndex, pivotIndex)
         .counters({{"comparisons", comparisons}, {"swaps", swaps}, {"depth", maxDepth}})
         .note("Recursing on " + range(low, pivotIndex - 1) + " and " + range(pivotIndex + 1, high) +
               ". The pivot doesn't enter either call.")
         .emit();

        quickSort(a, low, pivotIndex - 1);
        quickSort(a, pivotIndex + 1, high);
    } else {
        T.at(__LINE__, __func__)
         .vars({{"low", low}, {"high", high}})
         .counters({{"comparisons", comparisons}, {"swaps", swaps}, {"depth", maxDepth}})
         .note("Empty or single-element segment: nothing to do, unwinding.")
         .emit();
    }
    T.popFrame();
}

int main() {
    T.view("array").complexity("O(n log n) average, O(n²) worst", "O(log n)");
    T.panel("callstack", "Call stack");

    const std::vector<Dataset> cases = {
        {"random", {5, 3, 8, 6, 2, 7, 1, 4}},
        {"already sorted (degenerates to O(n²))", {1, 2, 3, 4, 5, 6, 7, 8}},
        {"reversed", {8, 7, 6, 5, 4, 3, 2, 1}},
        {"with duplicates", {4, 2, 4, 1, 3, 2, 4, 1}},
    };

    for (const Dataset& ds : cases) {
        int n = static_cast<int>(ds.data.size());
        int a[16];
        for (int i = 0; i < n; i++) a[i] = ds.data[i];

        comparisons = swaps = maxDepth = 0;
        T.input(ds.label);
        T.clearFrames().clearWatches().watchArray("a", a, n);
        T.at(__LINE__, "quickSort")
         .counters({{"comparisons", 0}, {"swaps", 0}, {"depth", 0}})
         .note("Starting array. Watch the maximum stack depth: it's what makes the difference "
               "between O(n log n) and O(n²).")
         .emit();

        quickSort(a, 0, n - 1);

        T.at(__LINE__, "quickSort")
         .region("sorted", 0, n - 1)
         .counters({{"comparisons", comparisons}, {"swaps", swaps}, {"depth", maxDepth}})
         .note("Sorted with " + std::to_string(comparisons) + " comparisons, maximum depth " +
               std::to_string(maxDepth) + " over " + std::to_string(n) + " elements.")
         .emit();
    }

    T.dump("traces/quick_sort.js");
    return 0;
}
