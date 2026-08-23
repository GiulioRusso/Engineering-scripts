// Binary Search
//
// Richiede un array ordinato. Ad ogni passo confronta la chiave con l'elemento
// centrale e scarta metà dell'intervallo di ricerca. La metà scartata resta
// disegnata in grigio: è lì che si vede da dove viene il logaritmo.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("binary_search", "Binary Search", "Array", __FILE__);
static int comparisons = 0;

int binarySearch(int a[], int n, int key) {
    int low = 0;
    int high = n - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        T.at(__LINE__, __func__)
         .vars({{"low", low}, {"high", high}, {"mid", mid}, {"key", key}, {"a[mid]", a[mid]}})
         .marks({{"low", low}, {"mid", mid}, {"high", high}})
         .region("excluded", 0, low - 1)
         .region("excluded", high + 1, n - 1)
         .region("window", low, high)
         .compare(mid, -1)
         .counters({{"confronti", ++comparisons}})
         .note("Intervallo [" + std::to_string(low) + ", " + std::to_string(high) + "], centro mid = " +
               std::to_string(mid) + ". Confronto a[mid] = " + std::to_string(a[mid]) + " con " +
               std::to_string(key) + ".")
         .emit();

        if (a[mid] == key) {
            T.at(__LINE__, __func__)
             .vars({{"low", low}, {"high", high}, {"mid", mid}, {"key", key}})
             .marks({{"low", low}, {"mid", mid}, {"high", high}})
             .region("excluded", 0, low - 1)
             .region("excluded", high + 1, n - 1)
             .found(mid)
             .counters({{"confronti", comparisons}})
             .note("a[mid] è la chiave: trovata in posizione " + std::to_string(mid) + " con soli " +
                   std::to_string(comparisons) + " confronti.")
             .emit();
            return mid;
        }

        if (a[mid] < key) {
            low = mid + 1;
            T.at(__LINE__, __func__)
             .vars({{"low", low}, {"high", high}, {"mid", mid}, {"key", key}})
             .marks({{"low", low}, {"high", high}})
             .region("excluded", 0, low - 1)
             .region("excluded", high + 1, n - 1)
             .region("window", low, high)
             .discard(0, low - 1)
             .counters({{"confronti", comparisons}})
             .note("a[mid] è troppo piccolo: scarto tutta la metà sinistra, low diventa " +
                   std::to_string(low) + ".")
             .emit();
        } else {
            high = mid - 1;
            T.at(__LINE__, __func__)
             .vars({{"low", low}, {"high", high}, {"mid", mid}, {"key", key}})
             .marks({{"low", low}, {"high", high}})
             .region("excluded", 0, low - 1)
             .region("excluded", high + 1, n - 1)
             .region("window", low, high)
             .discard(high + 1, n - 1)
             .counters({{"confronti", comparisons}})
             .note("a[mid] è troppo grande: scarto tutta la metà destra, high diventa " +
                   std::to_string(high) + ".")
             .emit();
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"low", low}, {"high", high}, {"key", key}})
     .region("excluded", 0, n - 1)
     .counters({{"confronti", comparisons}})
     .note("low ha superato high: l'intervallo è vuoto, la chiave non c'è. Ritorno -1.")
     .emit();
    return -1;
}

int main() {
    T.view("array").complexity("O(log n)", "O(1)");

    std::vector<int> data = trace::sortedData();
    int n = static_cast<int>(data.size());
    int a[16];
    for (int i = 0; i < n; i++) a[i] = data[i];

    struct Case { const char* label; int key; };
    const Case cases[] = {
        {"chiave presente (23)", 23},
        {"chiave assente (40)", 40},
        {"primo elemento (2)", 2},
        {"ultimo elemento (91)", 91},
    };

    for (const Case& c : cases) {
        comparisons = 0;
        T.input(c.label);
        T.clearWatches().watchArray("a", a, n);
        T.at(__LINE__, "binarySearch")
         .vars({{"key", c.key}})
         .region("window", 0, n - 1)
         .counters({{"confronti", 0}})
         .note(std::string("Array ordinato di ") + std::to_string(n) + " elementi, cerco " +
               std::to_string(c.key) + ". Bastano al più " + std::to_string(4) + " confronti.")
         .emit();

        binarySearch(a, n, c.key);
    }

    T.dump("traces/binary_search.js");
    return 0;
}
