// Heap: Delete (root and by index)
//
// Extracting the root is the operation a heap exists for: swap it with the
// last element, shrink the size, and sink the new head down.
//
// Deleting an arbitrary element is a different story: you first have to FIND
// it, and a heap isn't built for searching — there's no order between
// siblings, so the search is linear, O(n). Once the index is found, the
// value is set to INT_MIN (the sentinel), bubbled up to the root, and the
// root is extracted.
#include <climits>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("heap_delete", "Delete (root and by index)", "Heap", __FILE__);

static int comparisons = 0, swaps = 0;
static int heapArr[16];
static int heapSize = 0;

bool hasPriority(int a, int b) {
    comparisons++;
    return a < b;   // min-heap, as in the book
}

void heapify(int arr[], int n, int i) {
    T.pushFrame("heapify(i=" + std::to_string(i) + ")");
    int smallest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < n && hasPriority(arr[left], arr[smallest])) smallest = left;
    if (right < n && hasPriority(arr[right], arr[smallest])) smallest = right;

    T.at(__LINE__, __func__)
     .vars({{"i", i}, {"left", left}, {"right", right}, {"smallest", smallest}})
     .marks({{"i", i}, {"smallest", smallest}})
     .select(smallest)
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(i, "current").set(smallest != i ? smallest : -1, "path")))
     .counters({{"comparisons", comparisons}, {"swaps", swaps}})
     .note(smallest == i
             ? "Node " + std::to_string(i) + " already has priority over its children: the descent ends."
             : "The minimum among parent and children is at index " + std::to_string(smallest) + ": swap needed.")
     .emit();

    if (smallest != i) {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"smallest", smallest}})
         .marks({{"i", i}, {"smallest", smallest}})
         .swap(i, smallest)
         .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(i, "moved").set(smallest, "moved")))
         .counters({{"comparisons", comparisons}, {"swaps", ++swaps}})
         .note("Swap, and continue sinking from index " + std::to_string(smallest) + ".")
         .emit();

        int temp = arr[i];
        arr[i] = arr[smallest];
        arr[smallest] = temp;

        heapify(arr, n, smallest);
    }
    T.popFrame();
}

int extractMin(int arr[], int& n) {
    if (n <= 0) {
        T.at(__LINE__, __func__)
         .treeState(trace::heapTreeJson(arr, n))
         .error("underflow")
         .note("Heap empty: there's no minimum to extract.")
         .emit();
        return INT_MAX;
    }

    int root = arr[0];
    T.at(__LINE__, __func__)
     .vars({{"root", root}, {"n", n}})
     .marks({{"i", 0}})
     .select(0)
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(0, "current")))
     .counters({{"comparisons", comparisons}, {"swaps", swaps}})
     .note("The minimum is always the root, index 0: reading it costs O(1). All the work is in "
           "putting things back in order afterwards.")
     .emit();

    T.at(__LINE__, __func__)
     .vars({{"n", n}})
     .marks({{"i", 0}, {"last", n - 1}})
     .swap(0, n - 1)
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(0, "moved").set(n - 1, "moved")))
     .counters({{"comparisons", comparisons}, {"swaps", ++swaps}})
     .note("Moving the last leaf to the root. It's not a sensible candidate, but it's the only move "
           "that leaves the tree complete.")
     .emit();

    arr[0] = arr[n - 1];
    n--;
    T.at(__LINE__, __func__)
     .vars({{"n", n}})
     .marks({{"i", 0}})
     .select(0)
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(0, "current")))
     .counters({{"comparisons", comparisons}, {"swaps", swaps}})
     .note("n drops to " + std::to_string(n) + ": the last element leaves the heap simply by no "
           "longer being counted. The memory itself is untouched.")
     .emit();

    heapify(arr, n, 0);
    return root;
}

// Deletion by index: the INT_MIN sentinel bubbles up to the root by
// construction, because no value can ever have priority over it.
void deleteAtIndex(int arr[], int& n, int index) {
    if (index < 0 || index >= n) {
        T.at(__LINE__, __func__)
         .treeState(trace::heapTreeJson(arr, n))
         .error("out_of_range")
         .note("Index outside the heap.")
         .emit();
        return;
    }

    arr[index] = INT_MIN;
    T.at(__LINE__, __func__)
     .vars({{"index", index}, {"arr[index]", "INT_MIN"}})
     .marks({{"index", index}})
     .assign(index, 0)
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(index, "doomed").tag(index, "INT_MIN")))
     .counters({{"comparisons", comparisons}, {"swaps", swaps}})
     .note("Replacing the value with INT_MIN. From here on no parent can have priority over it, "
           "so the climb can't stop before the root.")
     .emit();

    int i = index;
    while (i != 0 && hasPriority(arr[i], arr[(i - 1) / 2])) {
        int parent = (i - 1) / 2;
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"parent", parent}})
         .marks({{"i", i}, {"parent", parent}})
         .swap(i, parent)
         .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(i, "doomed").set(parent, "moved")))
         .counters({{"comparisons", comparisons}, {"swaps", ++swaps}})
         .note("The sentinel climbs one level: index " + std::to_string(i) + " -> " +
               std::to_string(parent) + ".")
         .emit();

        int temp = arr[i];
        arr[i] = arr[parent];
        arr[parent] = temp;
        i = parent;
    }

    T.at(__LINE__, __func__)
     .marks({{"i", 0}})
     .error("sentinel")
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(0, "doomed").tag(0, "INT_MIN")))
     .counters({{"comparisons", comparisons}, {"swaps", swaps}})
     .note("The sentinel is at the root: a normal extract-min now removes it.")
     .emit();

    extractMin(arr, n);
}

// Searching a heap costs O(n): there's no order between siblings, so nothing
// can be ruled out. This is why a heap doesn't replace a BST.
int findIndex(int arr[], int n, int value) {
    for (int i = 0; i < n; i++) {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"value", value}, {"arr[i]", arr[i]}})
         .marks({{"i", i}})
         .compare(i, -1)
         .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(i, "current")))
         .counters({{"comparisons", comparisons}, {"swaps", swaps}})
         .note("Linear scan, index " + std::to_string(i) +
               ". A heap has no order between siblings: no subtree can be ruled out.")
         .emit();
        if (arr[i] == value) return i;
    }
    return -1;
}

static void run(const char* label, const std::vector<int>& initial, const char* intro) {
    heapSize = static_cast<int>(initial.size());
    for (int i = 0; i < heapSize; i++) heapArr[i] = initial[i];
    comparisons = swaps = 0;

    T.input(label);
    T.clearFrames().clearWatches().watchArray("heap", heapArr, heapSize);
    T.at(__LINE__, "extractMin")
     .vars({{"n", heapSize}})
     .treeState(trace::heapTreeJson(heapArr, heapSize))
     .counters({{"comparisons", 0}, {"swaps", 0}})
     .note(intro)
     .emit();
}

int main() {
    T.view("tree").complexity("O(log n) at the root, O(n) by index", "O(log n)");
    T.panel("array", "Array representation", "heap");
    T.panel("callstack", "Call stack");

    const std::vector<int> heap = {1, 3, 6, 5, 9, 8};

    run("extracting the minimum", heap, "A valid min-heap. Extracting the root.");
    extractMin(heapArr, heapSize);

    run("three extractions in a row", heap, "Extracting three times: the values come out in increasing order.");
    extractMin(heapArr, heapSize);
    T.clearFrames();
    extractMin(heapArr, heapSize);
    T.clearFrames();
    extractMin(heapArr, heapSize);

    run("deletion by value (8): O(n) search + sentinel", heap,
        "Deleting the value 8, which isn't the root. It takes three phases: find it, bubble it up, extract it.");
    {
        int index = findIndex(heapArr, heapSize, 8);
        T.clearFrames();
        deleteAtIndex(heapArr, heapSize, index);
    }

    run("extracting from a single-element heap", {42}, "A single element: the heap ends up empty.");
    extractMin(heapArr, heapSize);
    extractMin(heapArr, heapSize);

    T.dump("traces/heap_delete.js");
    return 0;
}
