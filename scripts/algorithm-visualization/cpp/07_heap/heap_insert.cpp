// Heap: Insert (bubble up)
//
// L'elemento nuovo va nella prima posizione libera dell'array, che nell'albero
// è il primo slot libero dell'ultimo livello: è l'unico posto che mantiene
// l'albero completo. Da lì risale finché ha priorità sul padre.
//
//   parent = (i - 1) / 2
//
// La risalita tocca al massimo un nodo per livello, quindi O(log n).
#include <climits>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("heap_insert", "Insert (bubble up)", "Heap", __FILE__);

static const int CAPACITY = 15;
static bool minHeap = true;
static int comparisons = 0, swaps = 0;

bool hasPriority(int a, int b) {
    comparisons++;
    return minHeap ? a < b : a > b;
}

static int heapArr[CAPACITY];
static int heapSize = 0;

void insertKey(int arr[], int& n, int value) {
    if (n == CAPACITY) {
        T.at(__LINE__, __func__)
         .vars({{"n", n}, {"capacity", CAPACITY}})
         .treeState(trace::heapTreeJson(arr, n))
         .error("overflow")
         .note("Heap pieno: non c'è una posizione libera nell'array.")
         .emit();
        return;
    }

    int i = n;
    arr[i] = value;
    n++;
    T.at(__LINE__, __func__)
     .vars({{"value", value}, {"i", i}, {"n", n}})
     .marks({{"i", i}})
     .assign(i, value)
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(i, "current")))
     .counters({{"confronti", comparisons}, {"scambi", swaps}})
     .note("Il nuovo valore " + std::to_string(value) + " va in fondo all'array, indice " +
           std::to_string(i) + ". Nell'albero è il primo slot libero dell'ultimo livello: l'albero "
           "resta completo, che è la condizione per poterlo tenere in un array senza buchi.")
     .emit();

    while (i != 0 && hasPriority(arr[i], arr[(i - 1) / 2])) {
        int parent = (i - 1) / 2;
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"parent", parent}, {"arr[i]", arr[i]}, {"arr[parent]", arr[parent]}})
         .marks({{"i", i}, {"parent", parent}})
         .swap(i, parent)
         .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(i, "moved").set(parent, "moved")))
         .counters({{"confronti", comparisons}, {"scambi", ++swaps}})
         .note("parent = (" + std::to_string(i) + "-1)/2 = " + std::to_string(parent) + ". Il figlio ha "
               "priorità sul padre, quindi salgono di posto: nell'array è uno scambio, nell'albero è "
               "una risalita di un livello.")
         .emit();

        int temp = arr[i];
        arr[i] = arr[parent];
        arr[parent] = temp;
        i = parent;
    }

    T.at(__LINE__, __func__)
     .vars({{"i", i}, {"n", n}})
     .marks({{"i", i}})
     .found(i)
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(i, "found")))
     .counters({{"confronti", comparisons}, {"scambi", swaps}})
     .note(i == 0 ? "Il valore è risalito fino alla radice: era il nuovo minimo (o massimo) dell'heap."
                  : "Il padre ha priorità: la risalita si ferma all'indice " + std::to_string(i) +
                    ". La proprietà dell'heap è di nuovo valida ovunque.")
     .emit();
}

static void run(const char* label, const std::vector<int>& initial, const std::vector<int>& toInsert,
                bool asMin, const char* intro) {
    heapSize = static_cast<int>(initial.size());
    for (int i = 0; i < heapSize; i++) heapArr[i] = initial[i];
    minHeap = asMin;
    comparisons = swaps = 0;

    T.input(label);
    T.clearWatches().watchArray("heap", heapArr, heapSize);
    T.at(__LINE__, "insertKey")
     .vars({{"n", heapSize}})
     .treeState(trace::heapTreeJson(heapArr, heapSize))
     .counters({{"confronti", 0}, {"scambi", 0}})
     .note(intro)
     .emit();

    for (int v : toInsert) insertKey(heapArr, heapSize, v);
}

int main() {
    T.view("tree").complexity("O(log n)", "O(1)");
    T.panel("array", "Rappresentazione ad array", "heap");

    run("inserimento che risale fino alla radice", {3, 5, 9, 6, 8, 20, 10}, {2}, true,
        "Min-heap valido di 7 elementi. Inserisco 2, che è più piccolo di tutto.");
    run("inserimento che si ferma subito", {3, 5, 9, 6, 8, 20, 10}, {15}, true,
        "Stesso heap. Inserisco 15: si ferma appena trova un padre che ha priorità su di lui.");
    run("costruzione per inserimenti successivi", {}, {9, 4, 7, 1, 8, 3, 6}, true,
        "Heap vuoto: lo costruisco un inserimento alla volta. Costa O(n log n), contro O(n) di buildHeap.");
    run("max-heap", {20, 8, 10, 6, 5, 3, 9}, {25}, false,
        "Max-heap. Inserisco 25, il nuovo massimo: stessa risalita, confronto rovesciato.");

    T.dump("traces/heap_insert.js");
    return 0;
}
