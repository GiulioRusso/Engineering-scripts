// Array: Insert
//
// Inserire in posizione k costa n-k spostamenti: gli elementi da k in poi
// devono scorrere a destra di una cella, uno alla volta, partendo dalla coda.
// Se si partisse dalla testa si sovrascriverebbero i valori non ancora copiati.
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
         .note("size ha raggiunto capacity: non c'è una cella libera, l'inserimento fallisce. "
               "Un array statico non cresce.")
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
         .counters({{"spostamenti", ++moves}})
         .note("Sposto a[" + std::to_string(j - 1) + "] in a[" + std::to_string(j) +
               "]. Si parte dalla coda proprio per non sovrascrivere.")
         .emit();
    }

    a[pos] = value;
    size++;
    T.at(__LINE__, __func__)
     .vars({{"pos", pos}, {"size", size}, {"capacity", CAPACITY}})
     .marks({{"pos", pos}})
     .region("empty", size, CAPACITY - 1)
     .assign(pos, value)
     .counters({{"spostamenti", moves}})
     .note("Il valore " + std::to_string(value) + " entra nella cella liberata in posizione " +
           std::to_string(pos) + ". size passa a " + std::to_string(size) + ".")
     .emit();
    return true;
}

int main() {
    T.view("array").complexity("O(n)", "O(1)");

    struct Case { const char* label; int pos; int value; int fill; };
    const Case cases[] = {
        {"inserimento in mezzo (pos 2)", 2, 99, 7},
        {"inserimento in testa (pos 0)", 0, 99, 7},
        {"inserimento in coda (pos = size)", 7, 99, 7},
        {"overflow (array pieno)", 2, 99, 10},
    };

    for (const Case& c : cases) {
        int a[CAPACITY] = {0};
        int size = c.fill;
        for (int i = 0; i < size; i++) a[i] = (i + 1) * 10;

        moves = 0;
        T.input(c.label);
        T.clearWatches().watchArray("a", a, CAPACITY);
        T.at(__LINE__, "insertAt")
         .vars({{"size", size}, {"capacity", CAPACITY}, {"pos", c.pos}, {"valore", c.value}})
         .marks({{"pos", c.pos}})
         .region("empty", size, CAPACITY - 1)
         .counters({{"spostamenti", 0}})
         .note("Array con size = " + std::to_string(size) + " su capacity = " + std::to_string(CAPACITY) +
               ". Inserisco " + std::to_string(c.value) + " in posizione " + std::to_string(c.pos) +
               ": serviranno " + std::to_string(size - c.pos) + " spostamenti.")
         .emit();

        insertAt(a, size, c.pos, c.value);

        T.at(__LINE__, "insertAt")
         .vars({{"size", size}})
         .region("empty", size, CAPACITY - 1)
         .counters({{"spostamenti", moves}})
         .note("Costo totale: " + std::to_string(moves) +
               " spostamenti. Inserire in testa è il caso peggiore, in coda costa zero.")
         .emit();
    }

    T.dump("traces/array_insert.js");
    return 0;
}
