// Array: Delete
//
// Cancellare in posizione k lascia un buco che va richiuso: gli elementi da k+1
// in poi scorrono a sinistra di una cella. Qui si parte dalla testa del buco,
// perché la direzione dello spostamento è opposta a quella dell'inserimento.
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
         .note("Posizione " + std::to_string(pos) + " non valida su size = " + std::to_string(size) +
               ": non c'è niente da cancellare.")
         .emit();
        return false;
    }

    T.at(__LINE__, __func__)
     .vars({{"pos", pos}, {"size", size}, {"a[pos]", a[pos]}})
     .marks({{"pos", pos}})
     .region("empty", size, CAPACITY - 1)
     .select(pos)
     .note("Cancello a[" + std::to_string(pos) + "] = " + std::to_string(a[pos]) +
           ". Il valore sparirà appena qualcosa lo sovrascrive.")
     .emit();

    for (int j = pos; j < size - 1; j++) {
        a[j] = a[j + 1];
        T.at(__LINE__, __func__)
         .vars({{"j", j}, {"pos", pos}, {"size", size}})
         .marks({{"pos", pos}, {"j", j}})
         .region("empty", size, CAPACITY - 1)
         .shift(j + 1, j, "left")
         .counters({{"spostamenti", ++moves}})
         .note("Sposto a[" + std::to_string(j + 1) + "] in a[" + std::to_string(j) +
               "]: il buco avanza verso destra.")
         .emit();
    }

    size--;
    a[size] = 0;
    T.at(__LINE__, __func__)
     .vars({{"pos", pos}, {"size", size}, {"capacity", CAPACITY}})
     .region("empty", size, CAPACITY - 1)
     .counters({{"spostamenti", moves}})
     .note("size scende a " + std::to_string(size) + ": l'ultima cella occupata torna libera. "
           "La memoria resta allocata, cambia solo quanta se ne considera valida.")
     .emit();
    return true;
}

int main() {
    T.view("array").complexity("O(n)", "O(1)");

    struct Case { const char* label; int pos; int fill; };
    const Case cases[] = {
        {"cancellazione in mezzo (pos 2)", 2, 7},
        {"cancellazione in testa (pos 0)", 0, 7},
        {"cancellazione in coda (pos 6)", 6, 7},
        {"posizione non valida", 7, 7},
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
         .counters({{"spostamenti", 0}})
         .note("Array con size = " + std::to_string(size) + ". Cancello la posizione " +
               std::to_string(c.pos) + ".")
         .emit();

        deleteAt(a, size, c.pos);
    }

    T.dump("traces/array_delete.js");
    return 0;
}
