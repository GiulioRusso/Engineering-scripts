// Array: Accessing
//
// Gli elementi sono contigui in memoria, quindi l'indirizzo dell'elemento i si
// calcola con una moltiplicazione e una somma: nessuna scansione. È questo che
// rende l'accesso O(1), indipendente da i e da n.
#include <cstdio>
#include <stdexcept>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("array_access", "Accessing", "Array", __FILE__);

const long BASE = 0x1000;          // indirizzo fittizio del primo elemento
const int ELEM_SIZE = sizeof(int);  // 4 byte

int accessAt(int a[], int n, int i) {
    if (i < 0 || i >= n) {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"n", n}})
         .error("out_of_range")
         .note("Indice " + std::to_string(i) + " fuori dall'intervallo [0, " + std::to_string(n - 1) +
               "]. L'aritmetica dell'indirizzo funzionerebbe lo stesso, e leggerebbe memoria di "
               "qualcun altro: per questo serve il controllo esplicito.")
         .emit();
        throw std::out_of_range("index " + std::to_string(i));
    }

    long offset = (long)i * ELEM_SIZE;
    T.at(__LINE__, __func__)
     .vars({{"i", i}, {"sizeof(int)", ELEM_SIZE}, {"offset", offset}})
     .marks({{"i", i}})
     .select(i)
     .note("offset = i * sizeof(int) = " + std::to_string(i) + " * " + std::to_string(ELEM_SIZE) +
           " = " + std::to_string(offset) + " byte.")
     .emit();

    long address = BASE + offset;
    T.at(__LINE__, __func__)
     .vars({{"i", i}, {"offset", offset}, {"indirizzo", address}, {"a[i]", a[i]}})
     .marks({{"i", i}})
     .found(i)
     .note("indirizzo = base + offset = " + std::to_string(BASE) + " + " + std::to_string(offset) +
           " = " + std::to_string(address) + ". Un solo calcolo, poi la lettura: a[" +
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
        {"accesso a metà (i = 3)", 3},
        {"primo elemento (i = 0)", 0},
        {"ultimo elemento (i = 7)", 7},
        {"indice fuori range (i = 9)", 9},
    };

    for (const Case& c : cases) {
        T.input(c.label);
        T.clearWatches().watchArray("a", a, n);
        T.at(__LINE__, "accessAt")
         .vars({{"base", BASE}, {"n", n}, {"i", c.index}})
         .note("Array di " + std::to_string(n) + " int contigui a partire dall'indirizzo " +
               std::to_string(BASE) + ". Chiedo l'elemento di indice " + std::to_string(c.index) + ".")
         .emit();

        try {
            accessAt(a, n, c.index);
        } catch (const std::out_of_range& e) {
            T.at(__LINE__, "accessAt")
             .vars({{"eccezione", std::string(e.what())}})
             .error("out_of_range")
             .note("std::out_of_range propagata al chiamante: l'accesso non avviene.")
             .emit();
        }
    }

    T.dump("traces/array_access.js");
    return 0;
}
