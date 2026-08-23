// Kadane
//
// Trova la somma massima di un sottoarray contiguo in una sola passata.
// L'intuizione: un prefisso con somma negativa non può mai aiutare il
// sottoarray che lo segue, quindi lo si butta e si riparte da zero.
#include <climits>
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/datasets.hpp"

static Tracer T("kadane", "Kadane", "Altro", __FILE__);

int maxSubarraySum(int a[], int n) {
    int maxSum = INT_MIN;
    int currentSum = 0;
    int start = 0, bestStart = 0, bestEnd = 0;

    for (int i = 0; i < n; i++) {
        currentSum = currentSum + a[i];
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"a[i]", a[i]}, {"currentSum", currentSum}, {"maxSum", maxSum == INT_MIN ? std::string("-inf") : std::to_string(maxSum)}})
         .marks({{"i", i}})
         .region("window", start, i)
         .region("sorted", bestStart, bestEnd)
         .select(i)
         .counters({{"passi", i + 1}})
         .note("Estendo il sottoarray corrente con a[" + std::to_string(i) + "] = " +
               std::to_string(a[i]) + ": currentSum = " + std::to_string(currentSum) + ".")
         .emit();

        if (currentSum > maxSum) {
            maxSum = currentSum;
            bestStart = start;
            bestEnd = i;
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"currentSum", currentSum}, {"maxSum", maxSum}})
             .marks({{"i", i}})
             .region("window", start, i)
             .region("sorted", bestStart, bestEnd)
             .found(i)
             .note("Nuovo massimo: maxSum = " + std::to_string(maxSum) + ", raggiunto dal sottoarray [" +
                   std::to_string(bestStart) + ", " + std::to_string(bestEnd) + "].")
             .emit();
        }

        if (currentSum < 0) {
            currentSum = 0;
            start = i + 1;
            T.at(__LINE__, __func__)
             .vars({{"i", i}, {"currentSum", currentSum}, {"maxSum", maxSum}, {"start", start}})
             .marks({{"i", i}})
             .region("excluded", 0, i)
             .region("sorted", bestStart, bestEnd)
             .discard(0, i)
             .note("currentSum è negativa: questo prefisso può solo peggiorare il seguito. "
                   "Azzero e riparto da a[" + std::to_string(start) + "]. È l'evento chiave di Kadane.")
             .emit();
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"maxSum", maxSum}})
     .region("sorted", bestStart, bestEnd)
     .note("Somma massima " + std::to_string(maxSum) + " sul sottoarray [" + std::to_string(bestStart) +
           ", " + std::to_string(bestEnd) + "], trovata in una sola passata: O(n) tempo, O(1) spazio.")
     .emit();
    return maxSum;
}

int main() {
    T.view("array").complexity("O(n)", "O(1)");

    const std::vector<Dataset> cases = {
        {"esempio classico", {-2, 1, -3, 4, -1, 2, 1, -5, 4}},
        {"tutti negativi", {-3, -1, -7, -2, -5}},
        {"tutti positivi", {2, 4, 1, 3, 5, 2}},
        {"massimo a cavallo di un negativo", {5, -2, 7, -8, 3, 4}},
    };

    for (const Dataset& ds : cases) {
        int n = static_cast<int>(ds.data.size());
        int a[16];
        for (int i = 0; i < n; i++) a[i] = ds.data[i];

        T.input(ds.label);
        T.clearWatches().watchArray("a", a, n);
        T.at(__LINE__, "maxSubarraySum")
         .vars({{"n", n}, {"currentSum", 0}, {"maxSum", "-inf"}})
         .counters({{"passi", 0}})
         .note("Array di partenza. currentSum e maxSum sono i due soli valori da tenere d'occhio.")
         .emit();

        maxSubarraySum(a, n);
    }

    T.dump("traces/kadane.js");
    return 0;
}
