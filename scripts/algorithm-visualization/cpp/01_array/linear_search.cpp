// Linear Search
//
// Scorre l'array dall'inizio alla fine confrontando ogni elemento con la
// chiave. Non richiede un array ordinato, ma nel caso peggiore (chiave assente)
// legge tutti gli n elementi.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("linear_search", "Linear Search", "Array", __FILE__);
static int comparisons = 0;

int linearSearch(int a[], int n, int key) {
    for (int i = 0; i < n; i++) {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"key", key}, {"a[i]", a[i]}})
         .marks({{"i", i}})
         .region("excluded", 0, i - 1)
         .compare(i, -1)
         .counters({{"confronti", ++comparisons}})
         .note("Confronto a[" + std::to_string(i) + "] = " + std::to_string(a[i]) +
               " con la chiave " + std::to_string(key) + ".")
         .emit();

        if (a[i] == key) {
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"key", key}})
             .marks({{"i", i}})
             .region("excluded", 0, i - 1)
             .found(i)
             .counters({{"confronti", comparisons}})
             .note("Trovata alla posizione " + std::to_string(i) + " dopo " +
                   std::to_string(comparisons) + " confronti.")
             .emit();
            return i;
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"key", key}})
     .region("excluded", 0, n - 1)
     .counters({{"confronti", comparisons}})
     .note("Fine array senza corrispondenza: ritorno -1 dopo " + std::to_string(comparisons) +
           " confronti. È il caso peggiore, O(n).")
     .emit();
    return -1;
}

int main() {
    T.view("array").complexity("O(n)", "O(1)");

    std::vector<int> data = trace::unsortedData();
    int n = static_cast<int>(data.size());
    int a[16];
    for (int i = 0; i < n; i++) a[i] = data[i];

    struct Case { const char* label; int key; };
    const Case cases[] = {
        {"caso migliore (primo elemento)", 23},
        {"caso medio (a metà)", 16},
        {"caso peggiore (chiave assente)", 40},
        {"ultimo elemento", 12},
    };

    for (const Case& c : cases) {
        comparisons = 0;
        T.input(c.label);
        T.clearWatches().watchArray("a", a, n);
        T.at(__LINE__, "linearSearch")
         .vars({{"key", c.key}})
         .counters({{"confronti", 0}})
         .note(std::string("Cerco la chiave ") + std::to_string(c.key) +
               " in un array non ordinato: nessuna scorciatoia, si parte da sinistra.")
         .emit();

        linearSearch(a, n, c.key);
    }

    T.dump("traces/linear_search.js");
    return 0;
}
