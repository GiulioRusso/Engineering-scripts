// Insertion Sort
//
// Tiene un prefisso ordinato e vi inserisce un elemento alla volta. L'elemento
// da collocare (key) viene estratto dall'array: il buco che lascia è quello in
// cui gli elementi maggiori scorrono a destra, uno per volta.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("insertion_sort", "Insertion Sort", "Ordinamento", __FILE__);
static int comparisons = 0, shifts = 0;

void insertionSort(int a[], int n) {
    for (int i = 1; i < n; i++) {
        int key = a[i];
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"key", key}})
         .marks({{"i", i}})
         .region("sorted", 0, i - 1)
         .region("hole", i, i)
         .lift("key", key, i)
         .counters({{"confronti", comparisons}, {"spostamenti", shifts}})
         .note("Estraggo key = a[" + std::to_string(i) + "] = " + std::to_string(key) +
               ". Il prefisso 0.." + std::to_string(i - 1) + " è già ordinato.")
         .emit();

        int j = i - 1;
        while (j >= 0 && a[j] > key) {
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"j", j}, {"key", key}, {"a[j]", a[j]}})
             .marks({{"i", i}, {"j", j}})
             .region("sorted", 0, i - 1)
             .region("hole", j + 1, j + 1)
             .lift("key", key, i)
             .compare(j, -1)
             .counters({{"confronti", ++comparisons}, {"spostamenti", shifts}})
             .note("a[" + std::to_string(j) + "] = " + std::to_string(a[j]) + " è maggiore di key = " +
                   std::to_string(key) + ": deve scorrere a destra.")
             .emit();

            a[j + 1] = a[j];
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"j", j}, {"key", key}})
             .marks({{"i", i}, {"j", j}})
             .region("sorted", 0, i - 1)
             .region("hole", j, j)
             .lift("key", key, i)
             .shift(j, j + 1, "right")
             .counters({{"confronti", comparisons}, {"spostamenti", ++shifts}})
             .note("Copio a[" + std::to_string(j) + "] in a[" + std::to_string(j + 1) +
                   "]: il buco si sposta a sinistra.")
             .emit();

            j--;
        }

        a[j + 1] = key;
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"j", j}, {"key", key}})
         .marks({{"i", i}, {"j", j}})
         .region("sorted", 0, i)
         .assign(j + 1, key)
         .counters({{"confronti", comparisons}, {"spostamenti", shifts}})
         .note("key = " + std::to_string(key) + " atterra in a[" + std::to_string(j + 1) +
               "]: il prefisso ordinato ora arriva a " + std::to_string(i) + ".")
         .emit();
    }
}

int main() {
    T.view("array").complexity("O(n^2)", "O(1)");

    std::vector<Dataset> inputs = trace::sortInputs();
    inputs.insert(inputs.begin(), {"esempio del libro", {5, 3, 8, 6, 2}});

    for (const Dataset& ds : inputs) {
        int n = static_cast<int>(ds.data.size());
        int a[16];
        for (int i = 0; i < n; i++) a[i] = ds.data[i];

        comparisons = shifts = 0;
        T.input(ds.label);
        T.clearWatches().watchArray("a", a, n);
        T.at(__LINE__, "insertionSort")
         .region("sorted", 0, 0)
         .counters({{"confronti", 0}, {"spostamenti", 0}})
         .note("Un array di un solo elemento è già ordinato: si parte da i = 1.")
         .emit();

        insertionSort(a, n);

        T.at(__LINE__, "insertionSort")
         .region("sorted", 0, n - 1)
         .counters({{"confronti", comparisons}, {"spostamenti", shifts}})
         .note("Ordinato con " + std::to_string(comparisons) + " confronti e " + std::to_string(shifts) +
               " spostamenti. Su input quasi ordinato il while esce subito: è il caso O(n).")
         .emit();
    }

    T.dump("traces/insertion_sort.js");
    return 0;
}
