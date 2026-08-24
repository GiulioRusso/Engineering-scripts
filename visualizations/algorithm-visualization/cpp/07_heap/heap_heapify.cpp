// Heap: Heapify (bubble down) and Building the Heap
//
// A heap is an array; the tree is just a way of reading it. Both views here
// are indispensable: the tree explains the property (every parent has
// priority over its children), the array explains the implementation, and
// the link between the two is entirely in three formulas.
//
//   left = 2i + 1      right = 2i + 2      parent = (i - 1) / 2
//
// buildHeap starts from the last non-leaf node, n/2 - 1: leaves are already
// valid one-element heaps and there's nothing to fix.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("heap_heapify", "Heapify (bubble down)", "Heap", __FILE__);

// The book implements the min-heap. The max-heap variant is the same code
// with the comparison reversed, so the direction of priority is a parameter.
static bool minHeap = true;
static int comparisons = 0, swaps = 0;

bool hasPriority(int a, int b) {
    comparisons++;
    return minHeap ? a < b : a > b;
}

static int heapArr[16];
static int heapSize = 0;

void heapify(int arr[], int n, int i) {
    T.pushFrame("heapify(i=" + std::to_string(i) + ")");
    int best = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;
    T.at(__LINE__, __func__)
     .vars({{"i", i}, {"left", left}, {"right", right}, {"n", n}})
     .marks({{"i", i}, {"left", left < n ? left : -1}, {"right", right < n ? right : -1}})
     .select(i)
     .treeState(trace::heapTreeJson(heapArr, heapSize,
                trace::IndexMarks().set(i, "current").set(left < n ? left : -1, "path").set(right < n ? right : -1, "path")))
     .counters({{"comparisons", comparisons}, {"swaps", swaps}})
     .note("Node i = " + std::to_string(i) + ". Children are computed, not searched for: left = 2*" +
           std::to_string(i) + "+1 = " + std::to_string(left) + ", right = 2*" + std::to_string(i) +
           "+2 = " + std::to_string(right) + ".")
     .emit();

    if (left < n && hasPriority(arr[left], arr[best])) {
        best = left;
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"left", left}, {"best", best}})
         .marks({{"i", i}, {"best", best}})
         .compare(left, i)
         .treeState(trace::heapTreeJson(heapArr, heapSize, trace::IndexMarks().set(i, "path").set(left, "current")))
         .counters({{"comparisons", comparisons}, {"swaps", swaps}})
         .note("The left child has priority over the parent: the candidate becomes index " +
               std::to_string(left) + ".")
         .emit();
    }

    if (right < n && hasPriority(arr[right], arr[best])) {
        best = right;
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"right", right}, {"best", best}})
         .marks({{"i", i}, {"best", best}})
         .compare(right, best)
         .treeState(trace::heapTreeJson(heapArr, heapSize, trace::IndexMarks().set(i, "path").set(right, "current")))
         .counters({{"comparisons", comparisons}, {"swaps", swaps}})
         .note("The right child has priority over the candidate too: best moves to index " +
               std::to_string(right) + ".")
         .emit();
    }

    if (best != i) {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"best", best}})
         .marks({{"i", i}, {"best", best}})
         .swap(i, best)
         .treeState(trace::heapTreeJson(heapArr, heapSize, trace::IndexMarks().set(i, "moved").set(best, "moved")))
         .counters({{"comparisons", comparisons}, {"swaps", ++swaps}})
         .note("Swap parent and child: the wrong value sinks one level. In the array it's a "
               "swap between cells " + std::to_string(i) + " and " + std::to_string(best) + ".")
         .emit();

        int temp = arr[i];
        arr[i] = arr[best];
        arr[best] = temp;

        heapify(arr, n, best);
    } else {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"best", best}})
         .marks({{"i", i}})
         .found(i)
         .treeState(trace::heapTreeJson(heapArr, heapSize, trace::IndexMarks().set(i, "found")))
         .counters({{"comparisons", comparisons}, {"swaps", swaps}})
         .note("Node " + std::to_string(i) + " already has priority over its children: the subtree "
               "is in order and the descent stops here.")
         .emit();
    }
    T.popFrame();
}

void buildHeap(int arr[], int n) {
    for (int i = n / 2 - 1; i >= 0; i--) {
        T.clearFrames();
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"n", n}})
         .marks({{"i", i}})
         .select(i)
         .treeState(trace::heapTreeJson(heapArr, heapSize, trace::IndexMarks().set(i, "current")))
         .counters({{"comparisons", comparisons}, {"swaps", swaps}})
         .note("Pass over i = " + std::to_string(i) + ". Starting from n/2-1 = " +
               std::to_string(n / 2 - 1) + " and working down: the leaves, from " + std::to_string(n / 2) +
               " onward, are already valid heaps and don't need touching.")
         .emit();
        heapify(arr, n, i);
    }
}

static void run(const char* label, const std::vector<int>& values, bool asMin, const char* intro) {
    heapSize = static_cast<int>(values.size());
    for (int i = 0; i < heapSize; i++) heapArr[i] = values[i];
    minHeap = asMin;
    comparisons = swaps = 0;

    T.input(label);
    T.clearFrames().clearWatches().watchArray("heap", heapArr, heapSize);
    T.at(__LINE__, "buildHeap")
     .treeState(trace::heapTreeJson(heapArr, heapSize))
     .counters({{"comparisons", 0}, {"swaps", 0}})
     .note(intro)
     .emit();

    buildHeap(heapArr, heapSize);

    T.clearFrames();
    T.at(__LINE__, "buildHeap")
     .treeState(trace::heapTreeJson(heapArr, heapSize, trace::IndexMarks().set(0, "found")))
     .counters({{"comparisons", comparisons}, {"swaps", swaps}})
     .note(std::string(asMin ? "Min-heap" : "Max-heap") + " built: the root now holds " +
           std::to_string(heapArr[0]) + ". Building the heap bottom-up costs O(n), not O(n log n).")
     .emit();
}

int main() {
    T.view("tree").complexity("O(log n) per heapify, O(n) for buildHeap", "O(log n)");
    T.panel("array", "Array representation", "heap");
    T.panel("callstack", "Call stack");

    run("min-heap (the book)", {9, 4, 7, 1, 8, 3, 6}, true,
        "An arbitrary array, read as a complete binary tree. No parent honours the property yet.");
    run("max-heap (same procedure, reversed comparison)", {9, 4, 7, 1, 8, 3, 6}, false,
        "Same array, same procedure: only the direction of the comparison changes.");
    run("array already a heap", {1, 3, 6, 4, 8, 7, 9}, true,
        "An array that's already a valid min-heap: no swaps, only comparisons.");
    run("worst case (minimum root has to sink all the way down)", {9, 8, 7, 6, 5, 4, 3}, true,
        "Decreasing order: the root has to sink all the way to the bottom.");

    T.dump("traces/heap_heapify.js");
    return 0;
}
