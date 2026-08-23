// Depth-First Search su grafo
//
// Va a fondo lungo un ramo finché può, poi torna indietro. La struttura che
// impone quest'ordine è lo stack: l'ultimo nodo impilato è il primo a uscire,
// quindi si scende sempre nel vicino appena scoperto.
//
// I vicini si impilano in ordine di indice DECRESCENTE. Sembra strano, ed è
// esattamente per compensare il LIFO: impilando F, E, D si estrae D per primo,
// cioè si esplora in ordine crescente come farebbe la versione ricorsiva.
//
// Il backtracking è il momento in cui si estrae un nodo i cui vicini sono già
// tutti visitati: lo stack si accorcia e la ricerca riprende da un ramo lasciato
// indietro.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"
#include "../tracer/graphs.hpp"

static Tracer T("graph_dfs", "Depth-First Search", "Grafi", __FILE__);

static trace::GraphData g;
static bool visited[8];
static int visitOrder[8];
static int visitCount = 0;

// Lo stack del capitolo Stack e Queue.
static const int SCAP = 12;
static int stackData[SCAP];
static int top = -1;

void push(int v) {
    top++;
    stackData[top] = v;
}

int pop() {
    int v = stackData[top];
    top--;
    return v;
}

// tracer-hide-begin
static bool inStack(int v) {
    for (int k = 0; k <= top; k++) if (stackData[k] == v) return true;
    return false;
}

static std::string graphSnapshot(int current, int probing) {
    trace::GraphJson gj;
    for (int i = 0; i < g.size(); i++) {
        const char* st = "";
        if (i == current) st = "current";
        else if (visitOrder[i] > 0) st = "visited";
        else if (inStack(i)) st = "frontier";
        gj.node(g.names[i], st);
    }
    for (int i = 0; i < g.size(); i++)
        for (int j = i + 1; j < g.size(); j++)
            if (g.adj[i][j]) {
                bool hot = (i == current && j == probing) || (j == current && i == probing);
                gj.edge(g.names[i], g.names[j], "", hot ? "current" : "");
            }
    return gj.str();
}

static std::string visitedTable() {
    std::string s = "{";
    for (int i = 0; i < g.size(); i++) {
        if (i) s += ",";
        s += trace::Val::quote(g.names[i]) + ":" +
             trace::Val::quote(visitOrder[i] > 0 ? std::to_string(visitOrder[i]) : "-");
    }
    return s + "}";
}

static std::string stackTable() {
    std::string s = "{";
    for (int i = 0; i < g.size(); i++) {
        if (i) s += ",";
        s += trace::Val::quote(g.names[i]) + ":" + trace::Val::quote(inStack(i) ? "impilato" : "");
    }
    return s + "}";
}

static std::string order() {
    std::string s;
    for (int k = 1; k <= visitCount; k++)
        for (int i = 0; i < g.size(); i++)
            if (visitOrder[i] == k) s += (s.empty() ? "" : " ") + g.names[i];
    return "Visited: " + (s.empty() ? "(vuoto)" : s);
}
// tracer-hide-end

void dfs(int start) {
    push(start);
    T.at(__LINE__, __func__)
     .vars({{"start", g.names[start]}, {"top", top}})
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("Visited", visitedTable())
     .tableRaw("Stack", stackTable())
     .push(start)
     .sequence(order())
     .counters({{"visitati", 0}})
     .note("Impilo la sorgente " + g.names[start] + ". Lo stack tiene i nodi scoperti ma non ancora visitati.")
     .emit();

    while (top >= 0) {
        int u = pop();
        T.at(__LINE__, __func__)
         .vars({{"u", g.names[u]}, {"top", top}})
         .graphState(graphSnapshot(u, -1))
         .tableRaw("Visited", visitedTable())
         .tableRaw("Stack", stackTable())
         .pop()
         .sequence(order())
         .counters({{"visitati", visitCount}})
         .note("Estraggo dalla cima: " + g.names[u] + ". Essendo l'ultimo impilato, è il più \"profondo\" "
               "fra quelli scoperti.")
         .emit();

        if (visited[u]) {
            T.at(__LINE__, __func__)
             .vars({{"u", g.names[u]}, {"top", top}})
             .graphState(graphSnapshot(u, -1))
             .tableRaw("Visited", visitedTable())
             .tableRaw("Stack", stackTable())
             .sequence(order())
             .counters({{"visitati", visitCount}})
             .note(g.names[u] + " era già stato visitato per un'altra strada: lo scarto. Un nodo può "
                   "finire nello stack più volte, e questo controllo è ciò che rende innocuo il doppione.")
             .emit();
            continue;
        }

        visited[u] = true;
        visitOrder[u] = ++visitCount;
        T.at(__LINE__, __func__)
         .vars({{"u", g.names[u]}, {"top", top}})
         .graphState(graphSnapshot(u, -1))
         .tableRaw("Visited", visitedTable())
         .tableRaw("Stack", stackTable())
         .visit(g.names[u])
         .sequence(order())
         .counters({{"visitati", visitCount}})
         .note("Visito " + g.names[u] + ", " + std::to_string(visitCount) + "° nodo.")
         .emit();

        int pushed = 0;
        for (int v = g.size() - 1; v >= 0; v--) {
            if (g.adj[u][v] == 0 || visited[v]) continue;
            push(v);
            pushed++;
            T.at(__LINE__, __func__)
             .vars({{"u", g.names[u]}, {"v", g.names[v]}, {"top", top}})
             .graphState(graphSnapshot(u, v))
             .tableRaw("Visited", visitedTable())
             .tableRaw("Stack", stackTable())
             .push(v)
             .sequence(order())
             .counters({{"visitati", visitCount}})
             .note("Impilo " + g.names[v] + ". Il ciclo va da " + std::to_string(g.size() - 1) +
                   " verso 0 proprio perché lo stack è LIFO: l'indice più piccolo, impilato per ultimo, "
                   "sarà il primo a uscire.")
             .emit();
        }

        if (pushed == 0) {
            T.at(__LINE__, __func__)
             .vars({{"u", g.names[u]}, {"top", top}})
             .graphState(graphSnapshot(u, -1))
             .tableRaw("Visited", visitedTable())
             .tableRaw("Stack", stackTable())
             .sequence(order())
             .counters({{"visitati", visitCount}})
             .note("Vicolo cieco: tutti i vicini di " + g.names[u] + " sono già visitati. Nessun "
                   "impilamento, quindi il prossimo giro estrae un nodo lasciato indietro: è il "
                   "BACKTRACKING.")
             .emit();
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"top", top}})
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("Visited", visitedTable())
     .tableRaw("Stack", stackTable())
     .sequence(order())
     .counters({{"visitati", visitCount}})
     .note("Stack vuoto: la visita è finita. Confronta l'ordine con quello della BFS sullo stesso "
           "grafo — stessi nodi, sequenza completamente diversa.")
     .emit();
}

static void run(const trace::GraphData& src, int start) {
    g = src;
    for (int i = 0; i < 8; i++) { visited[i] = false; visitOrder[i] = 0; }
    visitCount = 0;
    top = -1;
    for (int i = 0; i < SCAP; i++) stackData[i] = 0;

    T.input(g.label + ", sorgente " + g.names[start]);
    T.layout(g.layoutJson());
    T.watch("stack", [] {
        std::string s = "[";
        for (int i = 0; i < SCAP; i++) {
            if (i) s += ",";
            s += std::to_string(stackData[i]);   // indice del vertice, come nel codice
        }
        return s + "]";
    });
    T.at(__LINE__, "dfs")
     .vars({{"top", top}})
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("Visited", visitedTable())
     .tableRaw("Stack", stackTable())
     .sequence(order())
     .counters({{"visitati", 0}})
     .note("Grafo iniziale, stack vuoto (top = -1).")
     .emit();

    dfs(start);
}

int main() {
    T.view("graph").complexity("O(V + E) con lista, O(V²) con matrice", "O(V)");
    T.panel("stack", "Stack dei nodi scoperti", "stack");
    T.panel("table", "Visited e stack");

    run(trace::traversalGraph(), 0);
    run(trace::cycleGraph(), 0);
    run(trace::traversalGraph(), 5);

    T.dump("traces/graph_dfs.js");
    return 0;
}
