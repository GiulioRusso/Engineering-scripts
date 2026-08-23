// Dijkstra
//
// Cammini minimi da una sorgente, su archi di peso non negativo. L'idea: si
// estrae il nodo non ancora visitato con la distanza provvisoria più piccola,
// lo si dichiara definitivo, e si prova a migliorare i suoi vicini. Il nodo
// estratto è definitivo perché qualunque altro cammino dovrebbe passare da un
// nodo con distanza già maggiore, e i pesi non negativi non lo fanno più
// scendere. È esattamente questo il punto che salta con un arco negativo.
//
// Il codice del libro estrae il minimo con una scansione lineare, non con una
// coda a priorità: da qui il costo O(V²). È la versione implementata qui, anche
// se la prosa del libro parla di coda a priorità.
#include <climits>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"
#include "../tracer/graphs.hpp"

static Tracer T("dijkstra", "Dijkstra", "Cammini minimi", __FILE__);

static trace::GraphData g;
static int dist[8];
static bool visited[8];
static int parent[8];
static int relaxations = 0;

// tracer-hide-begin
static std::string distText(int v) {
    return dist[v] == INT_MAX ? "inf" : std::to_string(dist[v]);
}

static std::string distTable() {
    std::string s = "{";
    for (int i = 0; i < g.size(); i++) {
        if (i) s += ",";
        s += trace::Val::quote(g.names[i]) + ":" + trace::Val::quote(distText(i));
    }
    return s + "}";
}

static std::string visitedTable() {
    std::string s = "{";
    for (int i = 0; i < g.size(); i++) {
        if (i) s += ",";
        s += trace::Val::quote(g.names[i]) + ":" + trace::Val::quote(visited[i] ? "si" : "-");
    }
    return s + "}";
}

static std::string parentTable() {
    std::string s = "{";
    for (int i = 0; i < g.size(); i++) {
        if (i) s += ",";
        s += trace::Val::quote(g.names[i]) + ":" +
             trace::Val::quote(parent[i] < 0 ? "-" : g.names[parent[i]]);
    }
    return s + "}";
}

static std::string graphSnapshot(int u, int v) {
    trace::GraphJson gj;
    for (int i = 0; i < g.size(); i++) {
        const char* st = "";
        if (i == u) st = "current";
        else if (visited[i]) st = "visited";
        else if (dist[i] != INT_MAX) st = "frontier";
        gj.node(g.names[i], st);
    }
    for (int i = 0; i < g.size(); i++)
        for (int j = i + 1; j < g.size(); j++)
            if (g.adj[i][j]) {
                bool hot = (i == u && j == v) || (j == u && i == v);
                bool tree = parent[i] == j || parent[j] == i;
                gj.edge(g.names[i], g.names[j], std::to_string(g.adj[i][j]),
                        hot ? "current" : (tree ? "accepted" : ""));
            }
    return gj.str();
}
// tracer-hide-end

// Estrazione del minimo per scansione lineare: O(V) ogni volta, ed è quello che
// porta il totale a O(V²).
int minDistance() {
    int best = INT_MAX;
    int index = -1;
    for (int v = 0; v < g.size(); v++) {
        if (!visited[v] && dist[v] <= best) {
            best = dist[v];
            index = v;
        }
    }
    return index;
}

void dijkstra(int src) {
    for (int i = 0; i < g.size(); i++) {
        dist[i] = INT_MAX;
        visited[i] = false;
        parent[i] = -1;
    }
    dist[src] = 0;
    T.at(__LINE__, __func__)
     .vars({{"sorgente", g.names[src]}})
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("dist", distTable())
     .tableRaw("visited", visitedTable())
     .tableRaw("parent", parentTable())
     .counters({{"rilassamenti", relaxations}})
     .note("Tutte le distanze partono da infinito tranne la sorgente, che vale 0. Nessun nodo è "
           "ancora definitivo.")
     .emit();

    for (int passo = 0; passo < g.size(); passo++) {
        int u = minDistance();
        if (u < 0 || dist[u] == INT_MAX) break;

        T.at(__LINE__, __func__)
         .vars({{"u", g.names[u]}, {"dist[u]", distText(u)}})
         .graphState(graphSnapshot(u, -1))
         .tableRaw("dist", distTable())
         .tableRaw("visited", visitedTable())
         .tableRaw("parent", parentTable())
         .counters({{"rilassamenti", relaxations}})
         .note("Estraggo il non visitato con distanza minima: " + g.names[u] + " a " + distText(u) +
               ". Scansione lineare su tutti i V nodi, non coda a priorità.")
         .emit();

        visited[u] = true;
        T.at(__LINE__, __func__)
         .vars({{"u", g.names[u]}, {"dist[u]", distText(u)}})
         .graphState(graphSnapshot(u, -1))
         .tableRaw("dist", distTable())
         .tableRaw("visited", visitedTable())
         .tableRaw("parent", parentTable())
         .visit(g.names[u])
         .counters({{"rilassamenti", relaxations}})
         .note("Dichiaro " + g.names[u] + " definitivo. Nessun cammino potrà più accorciarlo: ogni "
               "alternativa dovrebbe passare per un nodo già più lontano, e con pesi non negativi da "
               "lì si può solo crescere.")
         .emit();

        for (int v = 0; v < g.size(); v++) {
            int w = g.adj[u][v];
            if (w == 0 || visited[v]) continue;

            if (dist[u] + w < dist[v]) {
                int before = dist[v];
                dist[v] = dist[u] + w;
                parent[v] = u;
                relaxations++;
                T.at(__LINE__, __func__)
                 .vars({{"u", g.names[u]}, {"v", g.names[v]}, {"w", w}, {"dist[v]", distText(v)}})
                 .graphState(graphSnapshot(u, v))
                 .tableRaw("dist", distTable())
                 .tableRaw("visited", visitedTable())
                 .tableRaw("parent", parentTable())
                 .relax(g.names[u], g.names[v], dist[v])
                 .counters({{"rilassamenti", relaxations}})
                 .note("RILASSAMENTO: dist[" + g.names[u] + "] + " + std::to_string(w) + " = " +
                       distText(v) + " < " +
                       (before == INT_MAX ? std::string("inf") : std::to_string(before)) +
                       ". Aggiorno dist[" + g.names[v] + "] e segno " + g.names[u] + " come suo predecessore.")
                 .emit();
            } else {
                T.at(__LINE__, __func__)
                 .vars({{"u", g.names[u]}, {"v", g.names[v]}, {"w", w}})
                 .graphState(graphSnapshot(u, v))
                 .tableRaw("dist", distTable())
                 .tableRaw("visited", visitedTable())
                 .tableRaw("parent", parentTable())
                 .counters({{"rilassamenti", relaxations}})
                 .note("Passare per " + g.names[u] + " costerebbe " +
                       std::to_string(dist[u] + w) + ", non meno di " + distText(v) +
                       ": nessun miglioramento, dist[" + g.names[v] + "] resta com'è.")
                 .emit();
            }
        }
    }

    T.at(__LINE__, __func__)
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("dist", distTable())
     .tableRaw("visited", visitedTable())
     .tableRaw("parent", parentTable())
     .counters({{"rilassamenti", relaxations}})
     .note("Finito. dist[] contiene le distanze minime dalla sorgente e parent[] l'albero dei cammini "
           "minimi, evidenziato in verde sul grafo.")
     .emit();
}

static void run(const trace::GraphData& src, int start) {
    g = src;
    relaxations = 0;
    T.input(g.label + ", sorgente " + g.names[start]);
    T.layout(g.layoutJson());
    dijkstra(start);
}

int main() {
    T.view("graph").complexity("O(V²) con scansione lineare", "O(V)");
    T.panel("table", "dist[], visited[] e parent[]");

    run(trace::weightedGraph(), 0);
    run(trace::weightedGraph(), 5);

    T.dump("traces/dijkstra.js");
    return 0;
}
