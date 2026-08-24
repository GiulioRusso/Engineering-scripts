// Binary Search
//
// Requires a sorted array. Each step compares the key against the middle
// element and discards half of the search interval. The discarded half stays
// drawn in grey: that's where the logarithm comes from.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("binary_search", "Binary Search", "Array", __FILE__);
static int comparisons = 0;

int binarySearch(int a[], int n, int key) {
    int low = 0;
    int high = n - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        T.at(__LINE__, __func__)
         .vars({{"low", low}, {"high", high}, {"mid", mid}, {"key", key}, {"a[mid]", a[mid]}})
         .marks({{"low", low}, {"mid", mid}, {"high", high}})
         .region("excluded", 0, low - 1)
         .region("excluded", high + 1, n - 1)
         .region("window", low, high)
         .compare(mid, -1)
         .counters({{"comparisons", ++comparisons}})
         .note("Interval [" + std::to_string(low) + ", " + std::to_string(high) + "], midpoint mid = " +
               std::to_string(mid) + ". Comparing a[mid] = " + std::to_string(a[mid]) + " with " +
               std::to_string(key) + ".")
         .emit();

        if (a[mid] == key) {
            T.at(__LINE__, __func__)
             .vars({{"low", low}, {"high", high}, {"mid", mid}, {"key", key}})
             .marks({{"low", low}, {"mid", mid}, {"high", high}})
             .region("excluded", 0, low - 1)
             .region("excluded", high + 1, n - 1)
             .found(mid)
             .counters({{"comparisons", comparisons}})
             .note("a[mid] is the key: found at position " + std::to_string(mid) + " in just " +
                   std::to_string(comparisons) + " comparisons.")
             .emit();
            return mid;
        }

        if (a[mid] < key) {
            low = mid + 1;
            T.at(__LINE__, __func__)
             .vars({{"low", low}, {"high", high}, {"mid", mid}, {"key", key}})
             .marks({{"low", low}, {"high", high}})
             .region("excluded", 0, low - 1)
             .region("excluded", high + 1, n - 1)
             .region("window", low, high)
             .discard(0, low - 1)
             .counters({{"comparisons", comparisons}})
             .note("a[mid] is too small: discarding the whole left half, low becomes " +
                   std::to_string(low) + ".")
             .emit();
        } else {
            high = mid - 1;
            T.at(__LINE__, __func__)
             .vars({{"low", low}, {"high", high}, {"mid", mid}, {"key", key}})
             .marks({{"low", low}, {"high", high}})
             .region("excluded", 0, low - 1)
             .region("excluded", high + 1, n - 1)
             .region("window", low, high)
             .discard(high + 1, n - 1)
             .counters({{"comparisons", comparisons}})
             .note("a[mid] is too big: discarding the whole right half, high becomes " +
                   std::to_string(high) + ".")
             .emit();
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"low", low}, {"high", high}, {"key", key}})
     .region("excluded", 0, n - 1)
     .counters({{"comparisons", comparisons}})
     .note("low has passed high: the interval is empty, the key isn't there. Returning -1.")
     .emit();
    return -1;
}

int main() {
    T.view("array").complexity("O(log n)", "O(1)");

    std::vector<int> data = trace::sortedData();
    int n = static_cast<int>(data.size());
    int a[16];
    for (int i = 0; i < n; i++) a[i] = data[i];

    struct Case { const char* label; int key; };
    const Case cases[] = {
        {"key present (23)", 23},
        {"key absent (40)", 40},
        {"first element (2)", 2},
        {"last element (91)", 91},
    };

    for (const Case& c : cases) {
        comparisons = 0;
        T.input(c.label);
        T.clearWatches().watchArray("a", a, n);
        T.at(__LINE__, "binarySearch")
         .vars({{"key", c.key}})
         .region("window", 0, n - 1)
         .counters({{"comparisons", 0}})
         .note(std::string("Sorted array of ") + std::to_string(n) + " elements, searching for " +
               std::to_string(c.key) + ". At most " + std::to_string(4) + " comparisons are needed.")
         .emit();

        binarySearch(a, n, c.key);
    }

    T.dump("traces/binary_search.js");
    return 0;
}
