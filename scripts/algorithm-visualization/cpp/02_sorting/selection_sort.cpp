// Selection Sort
//
// Ad ogni passata cerca il minimo della parte non ordinata e lo porta sul
// confine. I confronti sono sempre n(n-1)/2, qualunque sia l'input; a cambiare
// è solo il numero di scambi, al massimo uno per passata.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("selection_sort", "Selection Sort", "Ordinamento", __FILE__);
static int comparisons = 0, swaps = 0;

void selectionSort(int a[], int n) {
    for (int i = 0; i < n - 1; i++) {
        int minIndex = i;
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"minIndex", minIndex}, {"a[minIndex]", a[minIndex]}})
         .marks({{"i", i}, {"minIndex", minIndex}})
         .region("sorted", 0, i - 1)
         .select(minIndex)
         .counters({{"confronti", comparisons}, {"scambi", swaps}})
         .note("Nuova passata. Assumo che il minimo della parte non ordinata sia a[" +
               std::to_string(i) + "] = " + std::to_string(a[i]) + ".")
         .emit();

        for (int j = i + 1; j < n; j++) {
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"j", j}, {"minIndex", minIndex}, {"a[j]", a[j]}, {"a[minIndex]", a[minIndex]}})
             .marks({{"i", i}, {"j", j}, {"minIndex", minIndex}})
             .region("sorted", 0, i - 1)
             .compare(j, minIndex)
             .counters({{"confronti", ++comparisons}, {"scambi", swaps}})
             .note("Confronto a[" + std::to_string(j) + "] = " + std::to_string(a[j]) +
                   " con il minimo corrente a[" + std::to_string(minIndex) + "] = " +
                   std::to_string(a[minIndex]) + ".")
             .emit();

            if (a[j] < a[minIndex]) {
                minIndex = j;
                T.at(__LINE__, __func__)
                 .vars({{"i", i}, {"j", j}, {"minIndex", minIndex}, {"a[minIndex]", a[minIndex]}})
                 .marks({{"i", i}, {"j", j}, {"minIndex", minIndex}})
                 .region("sorted", 0, i - 1)
                 .select(minIndex)
                 .counters({{"confronti", comparisons}, {"scambi", swaps}})
                 .note("Più piccolo del minimo corrente: minIndex si sposta su " + std::to_string(j) + ".")
                 .emit();
            }
        }

        if (minIndex != i) {
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"minIndex", minIndex}})
             .marks({{"i", i}, {"minIndex", minIndex}})
             .region("sorted", 0, i - 1)
             .swap(i, minIndex)
             .counters({{"confronti", comparisons}, {"scambi", ++swaps}})
             .note("Scambio a[" + std::to_string(i) + "] con il minimo trovato a[" +
                   std::to_string(minIndex) + "]: è l'unico scambio di questa passata.")
             .emit();

            int temp = a[i];
            a[i] = a[minIndex];
            a[minIndex] = temp;
        }

        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"a[i]", a[i]}})
         .marks({{"i", i}})
         .region("sorted", 0, i)
         .counters({{"confronti", comparisons}, {"scambi", swaps}})
         .note("La posizione " + std::to_string(i) + " ora è definitiva: la parte ordinata cresce di un elemento.")
         .emit();
    }
}

int main() {
    T.view("array").complexity("O(n^2)", "O(1)");

    for (const Dataset& ds : trace::sortInputs()) {
        int n = static_cast<int>(ds.data.size());
        int a[16];
        for (int i = 0; i < n; i++) a[i] = ds.data[i];

        comparisons = swaps = 0;
        T.input(ds.label);
        T.clearWatches().watchArray("a", a, n);
        T.at(__LINE__, "selectionSort")
         .marks({{"i", 0}})
         .counters({{"confronti", 0}, {"scambi", 0}})
         .note("Array iniziale, tutto non ordinato.")
         .emit();

        selectionSort(a, n);

        T.at(__LINE__, "selectionSort")
         .region("sorted", 0, n - 1)
         .counters({{"confronti", comparisons}, {"scambi", swaps}})
         .note("Array ordinato con " + std::to_string(comparisons) + " confronti e " +
               std::to_string(swaps) + " scambi. I confronti non dipendono dall'input, gli scambi sì.")
         .emit();
    }

    T.dump("traces/selection_sort.js");
    return 0;
}
