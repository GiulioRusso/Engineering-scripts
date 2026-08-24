// Merge Sort
//
// Divide and conquer: split the array in half, sort the two halves
// recursively, then merge them. The merge works on two temporary arrays
// leftArr and rightArr and copies them back into a[] by comparing their
// heads: that's where all the work is.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("merge_sort", "Merge Sort", "Sorting", __FILE__);
static int arraySize = 0;
static int comparisons = 0, writes = 0;

static std::string range(int l, int r) { return "[" + std::to_string(l) + ", " + std::to_string(r) + "]"; }

void merge(int a[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    int leftArr[16], rightArr[16];

    for (int x = 0; x < n1; x++) leftArr[x] = a[left + x];
    for (int y = 0; y < n2; y++) rightArr[y] = a[mid + 1 + y];
    T.watchArray("leftArr", leftArr, n1).watchArray("rightArr", rightArr, n2);
    T.at(__LINE__, __func__)
     .vars({{"left", left}, {"mid", mid}, {"right", right}, {"n1", n1}, {"n2", n2}})
     .region("window", left, right)
     .counters({{"comparisons", comparisons}, {"writes", writes}})
     .note("Copying the two sorted halves " + range(left, mid) + " and " + range(mid + 1, right) +
           " into the two temporary arrays. The copy is needed because a[] is about to be overwritten.")
     .emit();

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"j", j}, {"k", k}, {"leftArr[i]", leftArr[i]}, {"rightArr[j]", rightArr[j]}})
         .marks({{"i", i}, {"j", j}, {"k", k}})
         .region("window", left, right)
         .region("sorted", left, k - 1)
         .compare(k, -1)
         .counters({{"comparisons", ++comparisons}, {"writes", writes}})
         .note("Comparing the two heads: leftArr[" + std::to_string(i) + "] = " + std::to_string(leftArr[i]) +
               " against rightArr[" + std::to_string(j) + "] = " + std::to_string(rightArr[j]) + ".")
         .emit();

        if (leftArr[i] <= rightArr[j]) {
            a[k] = leftArr[i];
            i++;
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"j", j}, {"k", k}})
             .marks({{"i", i}, {"j", j}, {"k", k}})
             .region("window", left, right)
             .region("sorted", left, k)
             .assign(k, a[k])
             .counters({{"comparisons", comparisons}, {"writes", ++writes}})
             .note("The left head wins: " + std::to_string(a[k]) + " goes into a[" + std::to_string(k) +
                   "]. The <= keeps the sort stable.")
             .emit();
        } else {
            a[k] = rightArr[j];
            j++;
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"j", j}, {"k", k}})
             .marks({{"i", i}, {"j", j}, {"k", k}})
             .region("window", left, right)
             .region("sorted", left, k)
             .assign(k, a[k])
             .counters({{"comparisons", comparisons}, {"writes", ++writes}})
             .note("The right head wins: " + std::to_string(a[k]) + " goes into a[" + std::to_string(k) + "].")
             .emit();
        }
        k++;
    }

    while (i < n1) {
        a[k] = leftArr[i];
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"k", k}})
         .marks({{"i", i}, {"k", k}})
         .region("window", left, right)
         .region("sorted", left, k)
         .assign(k, a[k])
         .counters({{"comparisons", comparisons}, {"writes", ++writes}})
         .note("rightArr is exhausted: copying the rest of leftArr with no more comparisons.")
         .emit();
        i++;
        k++;
    }

    while (j < n2) {
        a[k] = rightArr[j];
        T.at(__LINE__, __func__)
         .vars({{"j", j}, {"k", k}})
         .marks({{"j", j}, {"k", k}})
         .region("window", left, right)
         .region("sorted", left, k)
         .assign(k, a[k])
         .counters({{"comparisons", comparisons}, {"writes", ++writes}})
         .note("leftArr is exhausted: copying the rest of rightArr with no more comparisons.")
         .emit();
        j++;
        k++;
    }

    T.clearWatches().watchArray("a", a, arraySize);
    T.at(__LINE__, __func__)
     .region("sorted", left, right)
     .counters({{"comparisons", comparisons}, {"writes", writes}})
     .note("Merge complete: the segment " + range(left, right) + " is sorted.")
     .emit();
}

void mergeSort(int a[], int left, int right) {
    T.pushFrame("mergeSort(" + std::to_string(left) + ", " + std::to_string(right) + ")");
    if (left >= right) {
        T.at(__LINE__, __func__)
         .vars({{"left", left}, {"right", right}})
         .region("sorted", left, right)
         .counters({{"comparisons", comparisons}, {"writes", writes}})
         .note("Base case: a single-element segment is already sorted, unwinding.")
         .emit();
        T.popFrame();
        return;
    }

    int mid = left + (right - left) / 2;
    T.at(__LINE__, __func__)
     .vars({{"left", left}, {"mid", mid}, {"right", right}})
     .region("window", left, right)
     .counters({{"comparisons", comparisons}, {"writes", writes}})
     .note("Splitting " + range(left, right) + " into " + range(left, mid) + " and " + range(mid + 1, right) + ".")
     .emit();

    mergeSort(a, left, mid);
    mergeSort(a, mid + 1, right);
    merge(a, left, mid, right);
    T.popFrame();
}

int main() {
    T.view("array").complexity("O(n log n)", "O(n)");
    T.panel("array", "leftArr", "leftArr", "i");
    T.panel("array", "rightArr", "rightArr", "j");
    T.panel("callstack", "Call stack");

    for (const Dataset& ds : trace::sortInputs()) {
        int n = static_cast<int>(ds.data.size());
        int a[16];
        for (int i = 0; i < n; i++) a[i] = ds.data[i];

        arraySize = n;
        comparisons = writes = 0;
        T.input(ds.label);
        T.clearFrames().clearWatches().watchArray("a", a, arraySize);
        T.at(__LINE__, "mergeSort")
         .counters({{"comparisons", 0}, {"writes", 0}})
         .note("Starting array. Merge sort always splits in half: recursion depth is log₂ n.")
         .emit();

        mergeSort(a, 0, n - 1);

        T.at(__LINE__, "mergeSort")
         .region("sorted", 0, n - 1)
         .counters({{"comparisons", comparisons}, {"writes", writes}})
         .note("Sorted with " + std::to_string(comparisons) + " comparisons and " + std::to_string(writes) +
               " writes. The cost doesn't depend on the shape of the input: always O(n log n).")
         .emit();
    }

    T.dump("traces/merge_sort.js");
    return 0;
}
