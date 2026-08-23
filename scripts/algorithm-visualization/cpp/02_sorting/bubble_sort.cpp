// Bubble Sort
//
// Confronta coppie adiacenti e le scambia se sono fuori ordine. Ad ogni passata
// l'elemento più grande "risale" fino in fondo, quindi il bordo destro
// ordinato cresce. Il flag swapped permette l'uscita anticipata: su un array
// già ordinato basta una sola passata, cioè O(n).
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("bubble_sort", "Bubble Sort", "Ordinamento", __FILE__);
static int comparisons = 0, swaps = 0;

void bubbleSort(int a[], int n) {
    for (int i = 0; i < n - 1; i++) {
        bool swapped = false;
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"swapped", swapped}})
         .region("sorted", n - i, n - 1)
         .counters({{"confronti", comparisons}, {"scambi", swaps}})
         .note("Passata " + std::to_string(i + 1) + ": swapped riparte da false.")
         .emit();

        for (int j = 0; j < n - i - 1; j++) {
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"j", j}, {"a[j]", a[j]}, {"a[j+1]", a[j + 1]}, {"swapped", swapped}})
             .marks({{"j", j}})
             .region("sorted", n - i, n - 1)
             .region("window", j, j + 1)
             .compare(j, j + 1)
             .counters({{"confronti", ++comparisons}, {"scambi", swaps}})
             .note("Confronto la coppia adiacente a[" + std::to_string(j) + "] = " + std::to_string(a[j]) +
                   " e a[" + std::to_string(j + 1) + "] = " + std::to_string(a[j + 1]) + ".")
             .emit();

            if (a[j] > a[j + 1]) {
                T.at(__LINE__, __func__)
                 .vars({{"i", i}, {"j", j}, {"swapped", true}})
                 .marks({{"j", j}})
                 .region("sorted", n - i, n - 1)
                 .swap(j, j + 1)
                 .counters({{"confronti", comparisons}, {"scambi", ++swaps}})
                 .note("Fuori ordine: scambio i due elementi e alzo swapped.")
                 .emit();

                int temp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = temp;
                swapped = true;
            }
        }

        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"swapped", swapped}})
         .region("sorted", n - i - 1, n - 1)
         .counters({{"confronti", comparisons}, {"scambi", swaps}})
         .note(std::string("Fine passata: a[") + std::to_string(n - i - 1) +
               "] è al suo posto definitivo.")
         .emit();

        if (!swapped) {
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"swapped", swapped}})
             .region("sorted", 0, n - 1)
             .counters({{"confronti", comparisons}, {"scambi", swaps}})
             .note("Nessuno scambio in tutta la passata: l'array è già ordinato, esco in anticipo.")
             .emit();
            break;
        }
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
        T.at(__LINE__, "bubbleSort")
         .counters({{"confronti", 0}, {"scambi", 0}})
         .note("Array iniziale. Guarda il bordo destro: cresce di una cella per passata.")
         .emit();

        bubbleSort(a, n);

        T.at(__LINE__, "bubbleSort")
         .region("sorted", 0, n - 1)
         .counters({{"confronti", comparisons}, {"scambi", swaps}})
         .note("Ordinato con " + std::to_string(comparisons) + " confronti e " + std::to_string(swaps) +
               " scambi.")
         .emit();
    }

    T.dump("traces/bubble_sort.js");
    return 0;
}
