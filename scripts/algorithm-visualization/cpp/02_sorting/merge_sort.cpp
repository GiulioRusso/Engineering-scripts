// Merge Sort
//
// Divide et impera: spezza l'array a metà, ordina le due metà ricorsivamente,
// poi le fonde. La fusione lavora su due array temporanei leftArr e rightArr e
// li ricopia in a[] confrontando le teste: è lì che sta tutto il lavoro.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("merge_sort", "Merge Sort", "Ordinamento", __FILE__);
static int arraySize = 0;
static int comparisons = 0, writes = 0;

static std::string range(int l, int r) { return "[" + std::to_string(l) + ", " + std::to_string(r) + "]"; }

void merge(int a[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    int leftArr[16], rightArr[16];

    for (int x = 0; x < n1; x++) leftArr[x] = a[left + x];
    for (int y = 0; y < n2; y++) rightArr[y] = a[mid + 1 + y];
    T.watchArray("leftArr", leftArr, n1).watchArray("rightArr", rightArr, n2);
    T.at(__LINE__, __func__)
     .vars({{"left", left}, {"mid", mid}, {"right", right}, {"n1", n1}, {"n2", n2}})
     .region("window", left, right)
     .counters({{"confronti", comparisons}, {"scritture", writes}})
     .note("Copio le due metà ordinate " + range(left, mid) + " e " + range(mid + 1, right) +
           " nei due array temporanei. Serve la copia perché a[] verrà sovrascritto.")
     .emit();

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"j", j}, {"k", k}, {"leftArr[i]", leftArr[i]}, {"rightArr[j]", rightArr[j]}})
         .marks({{"i", i}, {"j", j}, {"k", k}})
         .region("window", left, right)
         .region("sorted", left, k - 1)
         .compare(k, -1)
         .counters({{"confronti", ++comparisons}, {"scritture", writes}})
         .note("Confronto le due teste: leftArr[" + std::to_string(i) + "] = " + std::to_string(leftArr[i]) +
               " contro rightArr[" + std::to_string(j) + "] = " + std::to_string(rightArr[j]) + ".")
         .emit();

        if (leftArr[i] <= rightArr[j]) {
            a[k] = leftArr[i];
            i++;
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"j", j}, {"k", k}})
             .marks({{"i", i}, {"j", j}, {"k", k}})
             .region("window", left, right)
             .region("sorted", left, k)
             .assign(k, a[k])
             .counters({{"confronti", comparisons}, {"scritture", ++writes}})
             .note("Vince la testa sinistra: " + std::to_string(a[k]) + " va in a[" + std::to_string(k) +
                   "]. Il <= mantiene stabile l'ordinamento.")
             .emit();
        } else {
            a[k] = rightArr[j];
            j++;
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"j", j}, {"k", k}})
             .marks({{"i", i}, {"j", j}, {"k", k}})
             .region("window", left, right)
             .region("sorted", left, k)
             .assign(k, a[k])
             .counters({{"confronti", comparisons}, {"scritture", ++writes}})
             .note("Vince la testa destra: " + std::to_string(a[k]) + " va in a[" + std::to_string(k) + "].")
             .emit();
        }
        k++;
    }

    while (i < n1) {
        a[k] = leftArr[i];
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"k", k}})
         .marks({{"i", i}, {"k", k}})
         .region("window", left, right)
         .region("sorted", left, k)
         .assign(k, a[k])
         .counters({{"confronti", comparisons}, {"scritture", ++writes}})
         .note("rightArr è esaurito: copio il resto di leftArr senza più confronti.")
         .emit();
        i++;
        k++;
    }

    while (j < n2) {
        a[k] = rightArr[j];
        T.at(__LINE__, __func__)
         .vars({{"j", j}, {"k", k}})
         .marks({{"j", j}, {"k", k}})
         .region("window", left, right)
         .region("sorted", left, k)
         .assign(k, a[k])
         .counters({{"confronti", comparisons}, {"scritture", ++writes}})
         .note("leftArr è esaurito: copio il resto di rightArr senza più confronti.")
         .emit();
        j++;
        k++;
    }

    T.clearWatches().watchArray("a", a, arraySize);
    T.at(__LINE__, __func__)
     .region("sorted", left, right)
     .counters({{"confronti", comparisons}, {"scritture", writes}})
     .note("Fusione completata: il segmento " + range(left, right) + " è ordinato.")
     .emit();
}

void mergeSort(int a[], int left, int right) {
    T.pushFrame("mergeSort(" + std::to_string(left) + ", " + std::to_string(right) + ")");
    if (left >= right) {
        T.at(__LINE__, __func__)
         .vars({{"left", left}, {"right", right}})
         .region("sorted", left, right)
         .counters({{"confronti", comparisons}, {"scritture", writes}})
         .note("Caso base: un segmento di un solo elemento è già ordinato, si torna indietro.")
         .emit();
        T.popFrame();
        return;
    }

    int mid = left + (right - left) / 2;
    T.at(__LINE__, __func__)
     .vars({{"left", left}, {"mid", mid}, {"right", right}})
     .region("window", left, right)
     .counters({{"confronti", comparisons}, {"scritture", writes}})
     .note("Divido " + range(left, right) + " in " + range(left, mid) + " e " + range(mid + 1, right) + ".")
     .emit();

    mergeSort(a, left, mid);
    mergeSort(a, mid + 1, right);
    merge(a, left, mid, right);
    T.popFrame();
}

int main() {
    T.view("array").complexity("O(n log n)", "O(n)");
    T.panel("array", "leftArr", "leftArr", "i");
    T.panel("array", "rightArr", "rightArr", "j");
    T.panel("callstack", "Stack delle chiamate");

    for (const Dataset& ds : trace::sortInputs()) {
        int n = static_cast<int>(ds.data.size());
        int a[16];
        for (int i = 0; i < n; i++) a[i] = ds.data[i];

        arraySize = n;
        comparisons = writes = 0;
        T.input(ds.label);
        T.clearFrames().clearWatches().watchArray("a", a, arraySize);
        T.at(__LINE__, "mergeSort")
         .counters({{"confronti", 0}, {"scritture", 0}})
         .note("Array iniziale. Merge sort divide sempre a metà: la profondità della ricorsione è log₂ n.")
         .emit();

        mergeSort(a, 0, n - 1);

        T.at(__LINE__, "mergeSort")
         .region("sorted", 0, n - 1)
         .counters({{"confronti", comparisons}, {"scritture", writes}})
         .note("Ordinato con " + std::to_string(comparisons) + " confronti e " + std::to_string(writes) +
               " scritture. Il costo non dipende dalla forma dell'input: sempre O(n log n).")
         .emit();
    }

    T.dump("traces/merge_sort.js");
    return 0;
}
