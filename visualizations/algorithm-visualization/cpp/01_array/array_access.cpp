// Array: Accessing
//
// Elements are contiguous in memory, so the address of element i is computed
// with one multiplication and one addition: no scanning. That is what makes
// access O(1), independent of both i and n.
#include <cstdio>
#include <stdexcept>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("array_access", "Accessing", "Array", __FILE__);

const long BASE = 0x1000;          // fictitious address of the first element
const int ELEM_SIZE = sizeof(int);  // 4 bytes

int accessAt(int a[], int n, int i) {
    if (i < 0 || i >= n) {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"n", n}})
         .error("out_of_range")
         .note("Index " + std::to_string(i) + " is out of range [0, " + std::to_string(n - 1) +
               "]. The address arithmetic would work just the same, and would read someone else's "
               "memory: that is why an explicit check is needed.")
         .emit();
        throw std::out_of_range("index " + std::to_string(i));
    }

    long offset = (long)i * ELEM_SIZE;
    T.at(__LINE__, __func__)
     .vars({{"i", i}, {"sizeof(int)", ELEM_SIZE}, {"offset", offset}})
     .marks({{"i", i}})
     .select(i)
     .note("offset = i * sizeof(int) = " + std::to_string(i) + " * " + std::to_string(ELEM_SIZE) +
           " = " + std::to_string(offset) + " bytes.")
     .emit();

    long address = BASE + offset;
    T.at(__LINE__, __func__)
     .vars({{"i", i}, {"offset", offset}, {"address", address}, {"a[i]", a[i]}})
     .marks({{"i", i}})
     .found(i)
     .note("address = base + offset = " + std::to_string(BASE) + " + " + std::to_string(offset) +
           " = " + std::to_string(address) + ". A single calculation, then the read: a[" +
           std::to_string(i) + "] = " + std::to_string(a[i]) + ".")
     .emit();

    return a[i];
}

int main() {
    T.view("array").complexity("O(1)", "O(1)");

    int a[8] = {10, 20, 30, 40, 50, 60, 70, 80};
    int n = 8;

    struct Case { const char* label; int index; };
    const Case cases[] = {
        {"middle access (i = 3)", 3},
        {"first element (i = 0)", 0},
        {"last element (i = 7)", 7},
        {"out-of-range index (i = 9)", 9},
    };

    for (const Case& c : cases) {
        T.input(c.label);
        T.clearWatches().watchArray("a", a, n);
        T.at(__LINE__, "accessAt")
         .vars({{"base", BASE}, {"n", n}, {"i", c.index}})
         .note("Array of " + std::to_string(n) + " contiguous ints starting at address " +
               std::to_string(BASE) + ". Requesting the element at index " + std::to_string(c.index) + ".")
         .emit();

        try {
            accessAt(a, n, c.index);
        } catch (const std::out_of_range& e) {
            T.at(__LINE__, "accessAt")
             .vars({{"exception", std::string(e.what())}})
             .error("out_of_range")
             .note("std::out_of_range propagates to the caller: the access never happens.")
             .emit();
        }
    }

    T.dump("traces/array_access.js");
    return 0;
}
