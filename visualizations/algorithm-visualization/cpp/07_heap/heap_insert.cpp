// Heap: Insert (bubble up)
//
// The new element goes into the array's first free slot, which in the tree is
// the last level's first free slot: it's the only spot that keeps the tree
// complete. From there it bubbles up as long as it has priority over its
// parent.
//
//   parent = (i - 1) / 2
//
// The climb touches at most one node per level, so O(log n).
#include <climits>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("heap_insert", "Insert (bubble up)", "Heap", __FILE__);

static const int CAPACITY = 15;
static bool minHeap = true;
static int comparisons = 0, swaps = 0;

bool hasPriority(int a, int b) {
    comparisons++;
    return minHeap ? a < b : a > b;
}

static int heapArr[CAPACITY];
static int heapSize = 0;

void insertKey(int arr[], int& n, int value) {
    if (n == CAPACITY) {
        T.at(__LINE__, __func__)
         .vars({{"n", n}, {"capacity", CAPACITY}})
         .treeState(trace::heapTreeJson(arr, n))
         .error("overflow")
         .note("Heap full: there's no free slot in the array.")
         .emit();
        return;
    }

    int i = n;
    arr[i] = value;
    n++;
    T.at(__LINE__, __func__)
     .vars({{"value", value}, {"i", i}, {"n", n}})
     .marks({{"i", i}})
     .assign(i, value)
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(i, "current")))
     .counters({{"comparisons", comparisons}, {"swaps", swaps}})
     .note("The new value " + std::to_string(value) + " goes at the end of the array, index " +
           std::to_string(i) + ". In the tree it's the first free slot of the last level: the tree "
           "stays complete, which is the condition for keeping it in an array with no gaps.")
     .emit();

    while (i != 0 && hasPriority(arr[i], arr[(i - 1) / 2])) {
        int parent = (i - 1) / 2;
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"parent", parent}, {"arr[i]", arr[i]}, {"arr[parent]", arr[parent]}})
         .marks({{"i", i}, {"parent", parent}})
         .swap(i, parent)
         .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(i, "moved").set(parent, "moved")))
         .counters({{"comparisons", comparisons}, {"swaps", ++swaps}})
         .note("parent = (" + std::to_string(i) + "-1)/2 = " + std::to_string(parent) + ". The child has "
               "priority over the parent, so they swap places: an array swap, and one level up in the "
               "tree.")
         .emit();

        int temp = arr[i];
        arr[i] = arr[parent];
        arr[parent] = temp;
        i = parent;
    }

    T.at(__LINE__, __func__)
     .vars({{"i", i}, {"n", n}})
     .marks({{"i", i}})
     .found(i)
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(i, "found")))
     .counters({{"comparisons", comparisons}, {"swaps", swaps}})
     .note(i == 0 ? "The value bubbled all the way up to the root: it was the new min (or max) of the heap."
                  : "The parent has priority: the climb stops at index " + std::to_string(i) +
                    ". The heap property holds everywhere again.")
     .emit();
}

static void run(const char* label, const std::vector<int>& initial, const std::vector<int>& toInsert,
                bool asMin, const char* intro) {
    heapSize = static_cast<int>(initial.size());
    for (int i = 0; i < heapSize; i++) heapArr[i] = initial[i];
    minHeap = asMin;
    comparisons = swaps = 0;

    T.input(label);
    T.clearWatches().watchArray("heap", heapArr, heapSize);
    T.at(__LINE__, "insertKey")
     .vars({{"n", heapSize}})
     .treeState(trace::heapTreeJson(heapArr, heapSize))
     .counters({{"comparisons", 0}, {"swaps", 0}})
     .note(intro)
     .emit();

    for (int v : toInsert) insertKey(heapArr, heapSize, v);
}

int main() {
    T.view("tree").complexity("O(log n)", "O(1)");
    T.panel("array", "Array representation", "heap");

    run("insertion that climbs to the root", {3, 5, 9, 6, 8, 20, 10}, {2}, true,
        "A valid 7-element min-heap. Inserting 2, which is smaller than everything.");
    run("insertion that stops right away", {3, 5, 9, 6, 8, 20, 10}, {15}, true,
        "Same heap. Inserting 15: it stops as soon as it finds a parent with priority over it.");
    run("building by successive insertions", {}, {9, 4, 7, 1, 8, 3, 6}, true,
        "Empty heap: building it one insertion at a time. Costs O(n log n), versus buildHeap's O(n).");
    run("max-heap", {20, 8, 10, 6, 5, 3, 9}, {25}, false,
        "Max-heap. Inserting 25, the new max: same climb, reversed comparison.");

    T.dump("traces/heap_insert.js");
    return 0;
}
