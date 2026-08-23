// Quick Sort
//
// Sceglie un pivot (qui l'ultimo elemento), partiziona l'array in "minori del
// pivot" e "non minori", poi ordina ricorsivamente le due parti. Il pivot
// finisce nella sua posizione definitiva e non si muove più.
//
// Su input già ordinato il pivot è sempre il massimo: una partizione resta
// vuota, la ricorsione diventa profonda n e il costo degenera a O(n²).
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("quick_sort", "Quick Sort", "Ordinamento", __FILE__);
static int comparisons = 0, swaps = 0, maxDepth = 0;

static std::string range(int l, int r) { return "[" + std::to_string(l) + ", " + std::to_string(r) + "]"; }

int partition(int a[], int low, int high) {
    int pivot = a[high];
    int i = low - 1;
    T.at(__LINE__, __func__)
     .vars({{"low", low}, {"high", high}, {"pivot", pivot}, {"i", i}})
     .marks({{"high", high}, {"i", i}})
     .region("window", low, high)
     .select(high)
     .counters({{"confronti", comparisons}, {"scambi", swaps}, {"profondita", maxDepth}})
     .note("Pivot = a[" + std::to_string(high) + "] = " + std::to_string(pivot) +
           ". i parte da low-1: è il confine della regione dei minori, per ora vuota.")
     .emit();

    for (int j = low; j < high; j++) {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"j", j}, {"pivot", pivot}, {"a[j]", a[j]}})
         .marks({{"i", i}, {"j", j}, {"high", high}})
         .region("window", low, high)
         .region("sorted", low, i)
         .compare(j, high)
         .counters({{"confronti", ++comparisons}, {"scambi", swaps}, {"profondita", maxDepth}})
         .note("a[" + std::to_string(j) + "] = " + std::to_string(a[j]) + " contro il pivot " +
               std::to_string(pivot) + ".")
         .emit();

        if (a[j] < pivot) {
            i++;
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"j", j}, {"pivot", pivot}})
             .marks({{"i", i}, {"j", j}, {"high", high}})
             .region("window", low, high)
             .region("sorted", low, i - 1)
             .swap(i, j)
             .counters({{"confronti", comparisons}, {"scambi", ++swaps}, {"profondita", maxDepth}})
             .note("Minore del pivot: allargo la regione dei minori a i = " + std::to_string(i) +
                   " e ci porto dentro a[" + std::to_string(j) + "].")
             .emit();

            int temp = a[i];
            a[i] = a[j];
            a[j] = temp;
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"i", i}, {"high", high}, {"pivot", pivot}})
     .marks({{"i", i}, {"high", high}})
     .region("window", low, high)
     .region("sorted", low, i)
     .swap(i + 1, high)
     .counters({{"confronti", comparisons}, {"scambi", ++swaps}, {"profondita", maxDepth}})
     .note("Momento decisivo: il pivot scavalca la regione dei minori e atterra in a[" +
           std::to_string(i + 1) + "]. Da qui in poi non si muoverà mai più.")
     .emit();

    int temp = a[i + 1];
    a[i + 1] = a[high];
    a[high] = temp;

    T.at(__LINE__, __func__)
     .vars({{"pivotIndex", i + 1}})
     .marks({{"pivot", i + 1}})
     .region("sorted", i + 1, i + 1)
     .found(i + 1)
     .counters({{"confronti", comparisons}, {"scambi", swaps}, {"profondita", maxDepth}})
     .note("Posizione definitiva del pivot: " + std::to_string(i + 1) + ". A sinistra i minori, a destra i maggiori.")
     .emit();
    return i + 1;
}

void quickSort(int a[], int low, int high) {
    T.pushFrame("quickSort(" + std::to_string(low) + ", " + std::to_string(high) + ")");
    if (T.depth() > maxDepth) maxDepth = T.depth();

    if (low < high) {
        int pivotIndex = partition(a, low, high);

        T.at(__LINE__, __func__)
         .vars({{"low", low}, {"high", high}, {"pivotIndex", pivotIndex}})
         .region("window", low, high)
         .region("sorted", pivotIndex, pivotIndex)
         .counters({{"confronti", comparisons}, {"scambi", swaps}, {"profondita", maxDepth}})
         .note("Ricorro su " + range(low, pivotIndex - 1) + " e " + range(pivotIndex + 1, high) +
               ". Il pivot non entra in nessuna delle due chiamate.")
         .emit();

        quickSort(a, low, pivotIndex - 1);
        quickSort(a, pivotIndex + 1, high);
    } else {
        T.at(__LINE__, __func__)
         .vars({{"low", low}, {"high", high}})
         .counters({{"confronti", comparisons}, {"scambi", swaps}, {"profondita", maxDepth}})
         .note("Segmento vuoto o di un solo elemento: niente da fare, si torna indietro.")
         .emit();
    }
    T.popFrame();
}

int main() {
    T.view("array").complexity("O(n log n) medio, O(n²) peggiore", "O(log n)");
    T.panel("callstack", "Stack delle chiamate");

    const std::vector<Dataset> cases = {
        {"casuale", {5, 3, 8, 6, 2, 7, 1, 4}},
        {"già ordinato (degenera a O(n²))", {1, 2, 3, 4, 5, 6, 7, 8}},
        {"invertito", {8, 7, 6, 5, 4, 3, 2, 1}},
        {"con duplicati", {4, 2, 4, 1, 3, 2, 4, 1}},
    };

    for (const Dataset& ds : cases) {
        int n = static_cast<int>(ds.data.size());
        int a[16];
        for (int i = 0; i < n; i++) a[i] = ds.data[i];

        comparisons = swaps = maxDepth = 0;
        T.input(ds.label);
        T.clearFrames().clearWatches().watchArray("a", a, n);
        T.at(__LINE__, "quickSort")
         .counters({{"confronti", 0}, {"scambi", 0}, {"profondita", 0}})
         .note("Array iniziale. Guarda la profondità massima dello stack: è quella a fare la differenza "
               "fra O(n log n) e O(n²).")
         .emit();

        quickSort(a, 0, n - 1);

        T.at(__LINE__, "quickSort")
         .region("sorted", 0, n - 1)
         .counters({{"confronti", comparisons}, {"scambi", swaps}, {"profondita", maxDepth}})
         .note("Ordinato con " + std::to_string(comparisons) + " confronti, profondità massima " +
               std::to_string(maxDepth) + " su " + std::to_string(n) + " elementi.")
         .emit();
    }

    T.dump("traces/quick_sort.js");
    return 0;
}
