// Array: Insert
//
// Inserting at position k costs n-k moves: the elements from k onward must
// shift one cell to the right, one at a time, starting from the tail.
// Starting from the head would overwrite values that haven't been copied yet.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("array_insert", "Insert", "Array", __FILE__);
static const int CAPACITY = 10;
static int moves = 0;

bool insertAt(int a[], int& size, int pos, int value) {
    if (size >= CAPACITY) {
        T.at(__LINE__, __func__)
         .vars({{"size", size}, {"capacity", CAPACITY}})
         .region("empty", size, CAPACITY - 1)
         .error("overflow")
         .note("size has reached capacity: there is no free cell, the insert fails. "
               "A static array does not grow.")
         .emit();
        return false;
    }

    for (int j = size; j > pos; j--) {
        a[j] = a[j - 1];
        T.at(__LINE__, __func__)
         .vars({{"j", j}, {"pos", pos}, {"size", size}})
         .marks({{"pos", pos}, {"j", j}})
         .region("empty", size + 1, CAPACITY - 1)
         .shift(j - 1, j, "right")
         .counters({{"moves", ++moves}})
         .note("Moving a[" + std::to_string(j - 1) + "] into a[" + std::to_string(j) +
               "]. Starting from the tail is exactly what avoids overwriting.")
         .emit();
    }

    a[pos] = value;
    size++;
    T.at(__LINE__, __func__)
     .vars({{"pos", pos}, {"size", size}, {"capacity", CAPACITY}})
     .marks({{"pos", pos}})
     .region("empty", size, CAPACITY - 1)
     .assign(pos, value)
     .counters({{"moves", moves}})
     .note("Value " + std::to_string(value) + " goes into the cell freed at position " +
           std::to_string(pos) + ". size becomes " + std::to_string(size) + ".")
     .emit();
    return true;
}

int main() {
    T.view("array").complexity("O(n)", "O(1)");

    struct Case { const char* label; int pos; int value; int fill; };
    const Case cases[] = {
        {"insert in the middle (pos 2)", 2, 99, 7},
        {"insert at the head (pos 0)", 0, 99, 7},
        {"insert at the tail (pos = size)", 7, 99, 7},
        {"overflow (full array)", 2, 99, 10},
    };

    for (const Case& c : cases) {
        int a[CAPACITY] = {0};
        int size = c.fill;
        for (int i = 0; i < size; i++) a[i] = (i + 1) * 10;

        moves = 0;
        T.input(c.label);
        T.clearWatches().watchArray("a", a, CAPACITY);
        T.at(__LINE__, "insertAt")
         .vars({{"size", size}, {"capacity", CAPACITY}, {"pos", c.pos}, {"value", c.value}})
         .marks({{"pos", c.pos}})
         .region("empty", size, CAPACITY - 1)
         .counters({{"moves", 0}})
         .note("Array with size = " + std::to_string(size) + " out of capacity = " + std::to_string(CAPACITY) +
               ". Inserting " + std::to_string(c.value) + " at position " + std::to_string(c.pos) +
               ": this will take " + std::to_string(size - c.pos) + " moves.")
         .emit();

        insertAt(a, size, c.pos, c.value);

        T.at(__LINE__, "insertAt")
         .vars({{"size", size}})
         .region("empty", size, CAPACITY - 1)
         .counters({{"moves", moves}})
         .note("Total cost: " + std::to_string(moves) +
               " moves. Inserting at the head is the worst case, at the tail it costs zero.")
         .emit();
    }

    T.dump("traces/array_insert.js");
    return 0;
}
