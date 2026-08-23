// Heap: Delete (radice e per indice)
//
// Estrarre la radice è l'operazione per cui l'heap esiste: la si scambia con
// l'ultimo elemento, si riduce la dimensione e si fa scendere il nuovo capo.
//
// Cancellare un elemento qualsiasi è un'altra storia: prima bisogna TROVARLO, e
// un heap non è fatto per cercare — non c'è nessun ordine fra fratelli, quindi
// la ricerca è lineare, O(n). Trovato l'indice, si porta il valore a INT_MIN
// (il sentinella), lo si fa risalire fino alla radice e si estrae la radice.
#include <climits>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("heap_delete", "Delete (root e per indice)", "Heap", __FILE__);

static int comparisons = 0, swaps = 0;
static int heapArr[16];
static int heapSize = 0;

bool hasPriority(int a, int b) {
    comparisons++;
    return a < b;   // min-heap, come nel libro
}

void heapify(int arr[], int n, int i) {
    T.pushFrame("heapify(i=" + std::to_string(i) + ")");
    int smallest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < n && hasPriority(arr[left], arr[smallest])) smallest = left;
    if (right < n && hasPriority(arr[right], arr[smallest])) smallest = right;

    T.at(__LINE__, __func__)
     .vars({{"i", i}, {"left", left}, {"right", right}, {"smallest", smallest}})
     .marks({{"i", i}, {"smallest", smallest}})
     .select(smallest)
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(i, "current").set(smallest != i ? smallest : -1, "path")))
     .counters({{"confronti", comparisons}, {"scambi", swaps}})
     .note(smallest == i
             ? "Il nodo " + std::to_string(i) + " ha già priorità sui figli: la discesa finisce."
             : "Il minimo fra padre e figli è all'indice " + std::to_string(smallest) + ": va scambiato.")
     .emit();

    if (smallest != i) {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"smallest", smallest}})
         .marks({{"i", i}, {"smallest", smallest}})
         .swap(i, smallest)
         .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(i, "moved").set(smallest, "moved")))
         .counters({{"confronti", comparisons}, {"scambi", ++swaps}})
         .note("Scambio e continuo a scendere dall'indice " + std::to_string(smallest) + ".")
         .emit();

        int temp = arr[i];
        arr[i] = arr[smallest];
        arr[smallest] = temp;

        heapify(arr, n, smallest);
    }
    T.popFrame();
}

int extractMin(int arr[], int& n) {
    if (n <= 0) {
        T.at(__LINE__, __func__)
         .treeState(trace::heapTreeJson(arr, n))
         .error("underflow")
         .note("Heap vuoto: non c'è nessun minimo da estrarre.")
         .emit();
        return INT_MAX;
    }

    int root = arr[0];
    T.at(__LINE__, __func__)
     .vars({{"root", root}, {"n", n}})
     .marks({{"i", 0}})
     .select(0)
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(0, "current")))
     .counters({{"confronti", comparisons}, {"scambi", swaps}})
     .note("Il minimo è sempre la radice, indice 0: leggerlo costa O(1). Il lavoro è tutto nel "
           "rimetterlo a posto dopo.")
     .emit();

    T.at(__LINE__, __func__)
     .vars({{"n", n}})
     .marks({{"i", 0}, {"ultimo", n - 1}})
     .swap(0, n - 1)
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(0, "moved").set(n - 1, "moved")))
     .counters({{"confronti", comparisons}, {"scambi", ++swaps}})
     .note("Porto l'ultima foglia in radice. Non è un candidato sensato, ma è l'unico spostamento che "
           "lascia l'albero completo.")
     .emit();

    arr[0] = arr[n - 1];
    n--;
    T.at(__LINE__, __func__)
     .vars({{"n", n}})
     .marks({{"i", 0}})
     .select(0)
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(0, "current")))
     .counters({{"confronti", comparisons}, {"scambi", swaps}})
     .note("n scende a " + std::to_string(n) + ": l'ultimo elemento esce dall'heap semplicemente non "
           "essendo più contato. La memoria non si tocca.")
     .emit();

    heapify(arr, n, 0);
    return root;
}

// La cancellazione per indice: il sentinella INT_MIN risale fino alla radice
// per costruzione, perché nessun valore può avere priorità su di lui.
void deleteAtIndex(int arr[], int& n, int index) {
    if (index < 0 || index >= n) {
        T.at(__LINE__, __func__)
         .treeState(trace::heapTreeJson(arr, n))
         .error("out_of_range")
         .note("Indice fuori dall'heap.")
         .emit();
        return;
    }

    arr[index] = INT_MIN;
    T.at(__LINE__, __func__)
     .vars({{"index", index}, {"arr[index]", "INT_MIN"}})
     .marks({{"index", index}})
     .assign(index, 0)
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(index, "doomed").tag(index, "INT_MIN")))
     .counters({{"confronti", comparisons}, {"scambi", swaps}})
     .note("Sostituisco il valore con INT_MIN. Da qui in poi nessun padre potrà avere priorità su di "
           "lui, quindi la risalita non può fermarsi prima della radice.")
     .emit();

    int i = index;
    while (i != 0 && hasPriority(arr[i], arr[(i - 1) / 2])) {
        int parent = (i - 1) / 2;
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"parent", parent}})
         .marks({{"i", i}, {"parent", parent}})
         .swap(i, parent)
         .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(i, "doomed").set(parent, "moved")))
         .counters({{"confronti", comparisons}, {"scambi", ++swaps}})
         .note("Il sentinella sale di un livello: indice " + std::to_string(i) + " → " +
               std::to_string(parent) + ".")
         .emit();

        int temp = arr[i];
        arr[i] = arr[parent];
        arr[parent] = temp;
        i = parent;
    }

    T.at(__LINE__, __func__)
     .marks({{"i", 0}})
     .error("sentinella")
     .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(0, "doomed").tag(0, "INT_MIN")))
     .counters({{"confronti", comparisons}, {"scambi", swaps}})
     .note("Il sentinella è in radice: adesso basta una normale estrazione del minimo per toglierlo.")
     .emit();

    extractMin(arr, n);
}

// Cercare in un heap costa O(n): fra due fratelli non c'è nessun ordine, quindi
// non si può scartare niente. È il motivo per cui un heap non sostituisce un BST.
int findIndex(int arr[], int n, int value) {
    for (int i = 0; i < n; i++) {
        T.at(__LINE__, __func__)
         .vars({{"i", i}, {"value", value}, {"arr[i]", arr[i]}})
         .marks({{"i", i}})
         .compare(i, -1)
         .treeState(trace::heapTreeJson(arr, n, trace::IndexMarks().set(i, "current")))
         .counters({{"confronti", comparisons}, {"scambi", swaps}})
         .note("Scansione lineare, indice " + std::to_string(i) +
               ". Un heap non è ordinato fra fratelli: nessun sottoalbero può essere escluso.")
         .emit();
        if (arr[i] == value) return i;
    }
    return -1;
}

static void run(const char* label, const std::vector<int>& initial, const char* intro) {
    heapSize = static_cast<int>(initial.size());
    for (int i = 0; i < heapSize; i++) heapArr[i] = initial[i];
    comparisons = swaps = 0;

    T.input(label);
    T.clearFrames().clearWatches().watchArray("heap", heapArr, heapSize);
    T.at(__LINE__, "extractMin")
     .vars({{"n", heapSize}})
     .treeState(trace::heapTreeJson(heapArr, heapSize))
     .counters({{"confronti", 0}, {"scambi", 0}})
     .note(intro)
     .emit();
}

int main() {
    T.view("tree").complexity("O(log n) sulla radice, O(n) per indice", "O(log n)");
    T.panel("array", "Rappresentazione ad array", "heap");
    T.panel("callstack", "Stack delle chiamate");

    const std::vector<int> heap = {1, 3, 6, 5, 9, 8};

    run("estrazione del minimo", heap, "Min-heap valido. Estraggo la radice.");
    extractMin(heapArr, heapSize);

    run("tre estrazioni di fila", heap, "Estraggo tre volte: i valori escono in ordine crescente.");
    extractMin(heapArr, heapSize);
    T.clearFrames();
    extractMin(heapArr, heapSize);
    T.clearFrames();
    extractMin(heapArr, heapSize);

    run("cancellazione per valore (8): ricerca O(n) + sentinella", heap,
        "Cancello il valore 8, che non è la radice. Servono tre fasi: trovarlo, farlo risalire, estrarlo.");
    {
        int index = findIndex(heapArr, heapSize, 8);
        T.clearFrames();
        deleteAtIndex(heapArr, heapSize, index);
    }

    run("estrazione da heap di un solo elemento", {42}, "Un solo elemento: l'heap resta vuoto.");
    extractMin(heapArr, heapSize);
    extractMin(heapArr, heapSize);

    T.dump("traces/heap_delete.js");
    return 0;
}
