// Insertion Sort
//
// Keeps a sorted prefix and inserts one element into it at a time. The
// element to place (key) is pulled out of the array: the hole it leaves is
// where the larger elements shift right, one at a time.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("insertion_sort", "Insertion Sort", "Sorting", __FILE__);
static int comparisons = 0, shifts = 0;

void insertionSort(int a[], int n) {
    for (int i = 1; i < n; i++) {
        int key = a[i];
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"key", key}})
         .marks({{"i", i}})
         .region("sorted", 0, i - 1)
         .region("hole", i, i)
         .lift("key", key, i)
         .counters({{"comparisons", comparisons}, {"shifts", shifts}})
         .note("Pulling out key = a[" + std::to_string(i) + "] = " + std::to_string(key) +
               ". The prefix 0.." + std::to_string(i - 1) + " is already sorted.")
         .emit();

        int j = i - 1;
        while (j >= 0 && a[j] > key) {
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"j", j}, {"key", key}, {"a[j]", a[j]}})
             .marks({{"i", i}, {"j", j}})
             .region("sorted", 0, i - 1)
             .region("hole", j + 1, j + 1)
             .lift("key", key, i)
             .compare(j, -1)
             .counters({{"comparisons", ++comparisons}, {"shifts", shifts}})
             .note("a[" + std::to_string(j) + "] = " + std::to_string(a[j]) + " is greater than key = " +
                   std::to_string(key) + ": it has to shift right.")
             .emit();

            a[j + 1] = a[j];
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"j", j}, {"key", key}})
             .marks({{"i", i}, {"j", j}})
             .region("sorted", 0, i - 1)
             .region("hole", j, j)
             .lift("key", key, i)
             .shift(j, j + 1, "right")
             .counters({{"comparisons", comparisons}, {"shifts", ++shifts}})
             .note("Copying a[" + std::to_string(j) + "] into a[" + std::to_string(j + 1) +
                   "]: the hole moves left.")
             .emit();

            j--;
        }

        a[j + 1] = key;
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"j", j}, {"key", key}})
         .marks({{"i", i}, {"j", j}})
         .region("sorted", 0, i)
         .assign(j + 1, key)
         .counters({{"comparisons", comparisons}, {"shifts", shifts}})
         .note("key = " + std::to_string(key) + " lands in a[" + std::to_string(j + 1) +
               "]: the sorted prefix now reaches " + std::to_string(i) + ".")
         .emit();
    }
}

int main() {
    T.view("array").complexity("O(n^2)", "O(1)");

    std::vector<Dataset> inputs = trace::sortInputs();
    inputs.insert(inputs.begin(), {"textbook example", {5, 3, 8, 6, 2}});

    for (const Dataset& ds : inputs) {
        int n = static_cast<int>(ds.data.size());
        int a[16];
        for (int i = 0; i < n; i++) a[i] = ds.data[i];

        comparisons = shifts = 0;
        T.input(ds.label);
        T.clearWatches().watchArray("a", a, n);
        T.at(__LINE__, "insertionSort")
         .region("sorted", 0, 0)
         .counters({{"comparisons", 0}, {"shifts", 0}})
         .note("A single-element array is already sorted: starting at i = 1.")
         .emit();

        insertionSort(a, n);

        T.at(__LINE__, "insertionSort")
         .region("sorted", 0, n - 1)
         .counters({{"comparisons", comparisons}, {"shifts", shifts}})
         .note("Sorted with " + std::to_string(comparisons) + " comparisons and " + std::to_string(shifts) +
               " shifts. On nearly-sorted input the while exits right away: this is the O(n) case.")
         .emit();
    }

    T.dump("traces/insertion_sort.js");
    return 0;
}
