// Heap: Heapify (bubble down) e costruzione dell'heap
//
// Un heap è un array; l'albero è solo il modo di leggerlo. Le due viste qui
// sono entrambe indispensabili: l'albero spiega la proprietà (ogni padre ha
// priorità sui figli), l'array spiega l'implementazione, e il legame fra le due
// è tutto in tre formule.
//
//   left = 2i + 1      right = 2i + 2      parent = (i - 1) / 2
//
// buildHeap parte dall'ultimo nodo non-foglia, cioè n/2 - 1: le foglie sono già
// heap validi di un solo elemento e non c'è niente da sistemare.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("heap_heapify", "Heapify (bubble down)", "Heap", __FILE__);

// Il libro implementa il min-heap. La variante max-heap è lo stesso codice con
// il confronto rovesciato, quindi il verso della priorità è un parametro.
static bool minHeap = true;
static int comparisons = 0, swaps = 0;

bool hasPriority(int a, int b) {
    comparisons++;
    return minHeap ? a < b : a > b;
}

static int heapArr[16];
static int heapSize = 0;

void heapify(int arr[], int n, int i) {
    T.pushFrame("heapify(i=" + std::to_string(i) + ")");
    int best = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;
    T.at(__LINE__, __func__)
     .vars({{"i", i}, {"left", left}, {"right", right}, {"n", n}})
     .marks({{"i", i}, {"left", left < n ? left : -1}, {"right", right < n ? right : -1}})
     .select(i)
     .treeState(trace::heapTreeJson(heapArr, heapSize,
                trace::IndexMarks().set(i, "current").set(left < n ? left : -1, "path").set(right < n ? right : -1, "path")))
     .counters({{"confronti", comparisons}, {"scambi", swaps}})
     .note("Nodo i = " + std::to_string(i) + ". I figli si calcolano, non si cercano: left = 2·" +
           std::to_string(i) + "+1 = " + std::to_string(left) + ", right = 2·" + std::to_string(i) +
           "+2 = " + std::to_string(right) + ".")
     .emit();

    if (left < n && hasPriority(arr[left], arr[best])) {
        best = left;
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"left", left}, {"best", best}})
         .marks({{"i", i}, {"best", best}})
         .compare(left, i)
         .treeState(trace::heapTreeJson(heapArr, heapSize, trace::IndexMarks().set(i, "path").set(left, "current")))
         .counters({{"confronti", comparisons}, {"scambi", swaps}})
         .note("Il figlio sinistro ha priorità sul padre: il candidato diventa l'indice " +
               std::to_string(left) + ".")
         .emit();
    }

    if (right < n && hasPriority(arr[right], arr[best])) {
        best = right;
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"right", right}, {"best", best}})
         .marks({{"i", i}, {"best", best}})
         .compare(right, best)
         .treeState(trace::heapTreeJson(heapArr, heapSize, trace::IndexMarks().set(i, "path").set(right, "current")))
         .counters({{"confronti", comparisons}, {"scambi", swaps}})
         .note("Il figlio destro ha priorità anche sul candidato: best passa all'indice " +
               std::to_string(right) + ".")
         .emit();
    }

    if (best != i) {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"best", best}})
         .marks({{"i", i}, {"best", best}})
         .swap(i, best)
         .treeState(trace::heapTreeJson(heapArr, heapSize, trace::IndexMarks().set(i, "moved").set(best, "moved")))
         .counters({{"confronti", comparisons}, {"scambi", ++swaps}})
         .note("Scambio padre e figlio: il valore sbagliato scende di un livello. Nell'array è uno "
               "scambio fra le celle " + std::to_string(i) + " e " + std::to_string(best) + ".")
         .emit();

        int temp = arr[i];
        arr[i] = arr[best];
        arr[best] = temp;

        heapify(arr, n, best);
    } else {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"best", best}})
         .marks({{"i", i}})
         .found(i)
         .treeState(trace::heapTreeJson(heapArr, heapSize, trace::IndexMarks().set(i, "found")))
         .counters({{"confronti", comparisons}, {"scambi", swaps}})
         .note("Il nodo " + std::to_string(i) + " ha già priorità sui suoi figli: il sottoalbero è a "
               "posto e la discesa si ferma qui.")
         .emit();
    }
    T.popFrame();
}

void buildHeap(int arr[], int n) {
    for (int i = n / 2 - 1; i >= 0; i--) {
        T.clearFrames();
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"n", n}})
         .marks({{"i", i}})
         .select(i)
         .treeState(trace::heapTreeJson(heapArr, heapSize, trace::IndexMarks().set(i, "current")))
         .counters({{"confronti", comparisons}, {"scambi", swaps}})
         .note("Passata su i = " + std::to_string(i) + ". Si parte da n/2-1 = " +
               std::to_string(n / 2 - 1) + " e si scende: le foglie, da " + std::to_string(n / 2) +
               " in poi, sono già heap validi e non vanno toccate.")
         .emit();
        heapify(arr, n, i);
    }
}

static void run(const char* label, const std::vector<int>& values, bool asMin, const char* intro) {
    heapSize = static_cast<int>(values.size());
    for (int i = 0; i < heapSize; i++) heapArr[i] = values[i];
    minHeap = asMin;
    comparisons = swaps = 0;

    T.input(label);
    T.clearFrames().clearWatches().watchArray("heap", heapArr, heapSize);
    T.at(__LINE__, "buildHeap")
     .treeState(trace::heapTreeJson(heapArr, heapSize))
     .counters({{"confronti", 0}, {"scambi", 0}})
     .note(intro)
     .emit();

    buildHeap(heapArr, heapSize);

    T.clearFrames();
    T.at(__LINE__, "buildHeap")
     .treeState(trace::heapTreeJson(heapArr, heapSize, trace::IndexMarks().set(0, "found")))
     .counters({{"confronti", comparisons}, {"scambi", swaps}})
     .note(std::string(asMin ? "Min-heap" : "Max-heap") + " costruito: la radice contiene ora " +
           std::to_string(heapArr[0]) + ". Costruire l'heap dal basso costa O(n), non O(n log n).")
     .emit();
}

int main() {
    T.view("tree").complexity("O(log n) per heapify, O(n) per buildHeap", "O(log n)");
    T.panel("array", "Rappresentazione ad array", "heap");
    T.panel("callstack", "Stack delle chiamate");

    run("min-heap (il libro)", {9, 4, 7, 1, 8, 3, 6}, true,
        "Array qualunque, letto come albero binario completo. Nessun padre rispetta ancora la proprietà.");
    run("max-heap (stessa procedura, confronto rovesciato)", {9, 4, 7, 1, 8, 3, 6}, false,
        "Stesso array, stessa procedura: cambia solo il verso del confronto.");
    run("array già heap", {1, 3, 6, 4, 8, 7, 9}, true,
        "Array che è già un min-heap valido: nessuno scambio, solo confronti.");
    run("caso peggiore (radice minima da far scendere)", {9, 8, 7, 6, 5, 4, 3}, true,
        "Ordine decrescente: la radice deve scendere fino in fondo.");

    T.dump("traces/heap_heapify.js");
    return 0;
}
