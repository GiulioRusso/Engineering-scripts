// Array: Delete
//
// Deleting at position k leaves a hole that has to be closed: the elements
// from k+1 onward shift one cell to the left. This starts from the head of
// the hole, because the direction of the shift is the opposite of insertion.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("array_delete", "Delete", "Array", __FILE__);
static const int CAPACITY = 10;
static int moves = 0;

bool deleteAt(int a[], int& size, int pos) {
    if (pos < 0 || pos >= size) {
        T.at(__LINE__, __func__)
         .vars({{"pos", pos}, {"size", size}})
         .region("empty", size, CAPACITY - 1)
         .error("underflow")
         .note("Position " + std::to_string(pos) + " is not valid for size = " + std::to_string(size) +
               ": there is nothing to delete.")
         .emit();
        return false;
    }

    T.at(__LINE__, __func__)
     .vars({{"pos", pos}, {"size", size}, {"a[pos]", a[pos]}})
     .marks({{"pos", pos}})
     .region("empty", size, CAPACITY - 1)
     .select(pos)
     .note("Deleting a[" + std::to_string(pos) + "] = " + std::to_string(a[pos]) +
           ". The value will disappear as soon as something overwrites it.")
     .emit();

    for (int j = pos; j < size - 1; j++) {
        a[j] = a[j + 1];
        T.at(__LINE__, __func__)
         .vars({{"j", j}, {"pos", pos}, {"size", size}})
         .marks({{"pos", pos}, {"j", j}})
         .region("empty", size, CAPACITY - 1)
         .shift(j + 1, j, "left")
         .counters({{"moves", ++moves}})
         .note("Moving a[" + std::to_string(j + 1) + "] into a[" + std::to_string(j) +
               "]: the hole advances to the right.")
         .emit();
    }

    size--;
    a[size] = 0;
    T.at(__LINE__, __func__)
     .vars({{"pos", pos}, {"size", size}, {"capacity", CAPACITY}})
     .region("empty", size, CAPACITY - 1)
     .counters({{"moves", moves}})
     .note("size drops to " + std::to_string(size) + ": the last occupied cell becomes free again. "
           "The memory stays allocated, only how much of it counts as valid changes.")
     .emit();
    return true;
}

int main() {
    T.view("array").complexity("O(n)", "O(1)");

    struct Case { const char* label; int pos; int fill; };
    const Case cases[] = {
        {"delete in the middle (pos 2)", 2, 7},
        {"delete at the head (pos 0)", 0, 7},
        {"delete at the tail (pos 6)", 6, 7},
        {"invalid position", 7, 7},
    };

    for (const Case& c : cases) {
        int a[CAPACITY] = {0};
        int size = c.fill;
        for (int i = 0; i < size; i++) a[i] = (i + 1) * 10;

        moves = 0;
        T.input(c.label);
        T.clearWatches().watchArray("a", a, CAPACITY);
        T.at(__LINE__, "deleteAt")
         .vars({{"size", size}, {"capacity", CAPACITY}, {"pos", c.pos}})
         .marks({{"pos", c.pos}})
         .region("empty", size, CAPACITY - 1)
         .counters({{"moves", 0}})
         .note("Array with size = " + std::to_string(size) + ". Deleting position " +
               std::to_string(c.pos) + ".")
         .emit();

        deleteAt(a, size, c.pos);
    }

    T.dump("traces/array_delete.js");
    return 0;
}
