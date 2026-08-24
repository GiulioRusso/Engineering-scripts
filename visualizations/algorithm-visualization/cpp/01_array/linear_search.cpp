// Linear Search
//
// Scans the array from start to end, comparing every element with the key.
// It doesn't require a sorted array, but in the worst case (key absent) it
// reads all n elements.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("linear_search", "Linear Search", "Array", __FILE__);
static int comparisons = 0;

int linearSearch(int a[], int n, int key) {
    for (int i = 0; i < n; i++) {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"key", key}, {"a[i]", a[i]}})
         .marks({{"i", i}})
         .region("excluded", 0, i - 1)
         .compare(i, -1)
         .counters({{"comparisons", ++comparisons}})
         .note("Comparing a[" + std::to_string(i) + "] = " + std::to_string(a[i]) +
               " with key " + std::to_string(key) + ".")
         .emit();

        if (a[i] == key) {
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"key", key}})
             .marks({{"i", i}})
             .region("excluded", 0, i - 1)
             .found(i)
             .counters({{"comparisons", comparisons}})
             .note("Found at position " + std::to_string(i) + " after " +
                   std::to_string(comparisons) + " comparisons.")
             .emit();
            return i;
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"key", key}})
     .region("excluded", 0, n - 1)
     .counters({{"comparisons", comparisons}})
     .note("End of array with no match: returning -1 after " + std::to_string(comparisons) +
           " comparisons. This is the worst case, O(n).")
     .emit();
    return -1;
}

int main() {
    T.view("array").complexity("O(n)", "O(1)");

    std::vector<int> data = trace::unsortedData();
    int n = static_cast<int>(data.size());
    int a[16];
    for (int i = 0; i < n; i++) a[i] = data[i];

    struct Case { const char* label; int key; };
    const Case cases[] = {
        {"best case (first element)", 23},
        {"average case (in the middle)", 16},
        {"worst case (key absent)", 40},
        {"last element", 12},
    };

    for (const Case& c : cases) {
        comparisons = 0;
        T.input(c.label);
        T.clearWatches().watchArray("a", a, n);
        T.at(__LINE__, "linearSearch")
         .vars({{"key", c.key}})
         .counters({{"comparisons", 0}})
         .note(std::string("Searching for key ") + std::to_string(c.key) +
               " in an unsorted array: no shortcut, starting from the left.")
         .emit();

        linearSearch(a, n, c.key);
    }

    T.dump("traces/linear_search.js");
    return 0;
}
