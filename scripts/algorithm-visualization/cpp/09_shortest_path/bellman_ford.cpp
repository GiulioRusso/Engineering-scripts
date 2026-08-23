// Bellman-Ford
//
// Cammini minimi da una sorgente, e a differenza di Dijkstra funziona anche con
// archi di peso negativo. In cambio è più lento: invece di scegliere ogni volta
// il nodo migliore, rilassa TUTTI gli archi, e lo fa V-1 volte.
//
// Perché proprio V-1? Un cammino minimo semplice tocca al più V nodi, quindi ha
// al più V-1 archi. Dopo la passata numero p, tutte le distanze raggiungibili
// con p archi sono corrette; dopo V-1 passate, tutte.
//
// Se una V-esima passata riesce ancora a migliorare qualcosa, allora esiste un
// cammino con V archi o più che continua ad accorciarsi: un CICLO NEGATIVO, e
// il cammino minimo semplicemente non esiste.
#include <climits>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"
#include "../tracer/graphs.hpp"

static Tracer T("bellman_ford", "Bellman-Ford", "Cammini minimi", __FILE__);

static const int INF = 1000000;

struct Edge {
    int u, v, w;
};

static trace::GraphData g;
static std::vector<Edge> edges;
static int dist[8];
static int parent[8];
static int relaxations = 0;

// tracer-hide-begin
static std::string distText(int v) { return dist[v] >= INF ? "inf" : std::to_string(dist[v]); }

static std::string rowJson() {
    std::string s = "{";
    for (int i = 0; i < g.size(); i++) {
        if (i) s += ",";
        s += trace::Val::quote(g.names[i]) + ":" + trace::Val::quote(distText(i));
    }
    return s + "}";
}

static std::vector<std::pair<std::string, std::string>> history;

static std::string graphSnapshot(int eu, int ev, const char* state) {
    trace::GraphJson gj;
    for (int i = 0; i < g.size(); i++) {
        const char* st = "";
        if (i == ev) st = "current";
        else if (i == eu) st = "frontier";
        else if (dist[i] < INF) st = "visited";
        gj.node(g.names[i], st);
    }
    for (const Edge& e : edges) {
        bool hot = (e.u == eu && e.v == ev);
        bool tree = parent[e.v] == e.u;
        gj.edge(g.names[e.u], g.names[e.v], std::to_string(e.w),
                hot ? state : (tree ? "accepted" : ""), true);
    }
    return gj.str();
}
// La tabella del libro: una riga per passata, una colonna per vertice.
static void emitTables(trace::StepBuilder& b) {
    for (const auto& row : history) b.tableRaw(row.first, row.second);
}
// tracer-hide-end

// La tabella del libro: una riga per passata, una colonna per vertice.

bool bellmanFord(int src) {
    for (int i = 0; i < g.size(); i++) {
        dist[i] = INF;
        parent[i] = -1;
    }
    dist[src] = 0;
    history.clear();
    history.push_back({"iniziale", rowJson()});
    {
        auto b = T.at(__LINE__, __func__);
        b.vars({{"sorgente", g.names[src]}, {"V", g.size()}, {"E", (int)edges.size()}})
         .graphState(graphSnapshot(-1, -1, ""))
         .counters({{"rilassamenti", relaxations}})
         .note("Distanze a infinito tranne la sorgente. Serviranno " + std::to_string(g.size() - 1) +
               " passate complete su tutti gli " + std::to_string(edges.size()) + " archi.");
        emitTables(b);
        b.emit();
    }

    for (int pass = 1; pass <= g.size() - 1; pass++) {
        bool changed = false;

        for (const Edge& e : edges) {
            if (dist[e.u] < INF && dist[e.u] + e.w < dist[e.v]) {
                int before = dist[e.v];
                dist[e.v] = dist[e.u] + e.w;
                parent[e.v] = e.u;
                relaxations++;
                changed = true;
                auto b = T.at(__LINE__, __func__);
                b.vars({{"passata", pass}, {"u", g.names[e.u]}, {"v", g.names[e.v]}, {"w", e.w}})
                 .graphState(graphSnapshot(e.u, e.v, "current"))
                 .relax(g.names[e.u], g.names[e.v], dist[e.v])
                 .counters({{"rilassamenti", relaxations}})
                 .note("Passata " + std::to_string(pass) + ", arco " + g.names[e.u] + "->" +
                       g.names[e.v] + " (peso " + std::to_string(e.w) + "): " + distText(e.u) + " + " +
                       std::to_string(e.w) + " = " + distText(e.v) + ", meglio di " +
                       (before >= INF ? std::string("inf") : std::to_string(before)) + ".");
                emitTables(b);
                b.emit();
            } else {
                auto b = T.at(__LINE__, __func__);
                b.vars({{"passata", pass}, {"u", g.names[e.u]}, {"v", g.names[e.v]}, {"w", e.w}})
                 .graphState(graphSnapshot(e.u, e.v, "rejected"))
                 .counters({{"rilassamenti", relaxations}})
                 .note("Arco " + g.names[e.u] + "->" + g.names[e.v] +
                       ": nessun miglioramento. Bellman-Ford guarda ogni arco a ogni passata, anche "
                       "quelli che non serviranno mai.");
                emitTables(b);
                b.emit();
            }
        }

        history.push_back({"passata " + std::to_string(pass), rowJson()});
        {
            auto b = T.at(__LINE__, __func__);
            b.vars({{"passata", pass}, {"cambiato", changed}})
             .graphState(graphSnapshot(-1, -1, ""))
             .counters({{"rilassamenti", relaxations}})
             .note("Fine passata " + std::to_string(pass) + ". Ora sono corrette tutte le distanze "
                   "raggiungibili con al più " + std::to_string(pass) + " archi." +
                   (changed ? "" : " Nessun cambiamento: le passate successive non cambieranno nulla."));
            emitTables(b);
            b.emit();
        }
    }

    // Passata di rilevamento: la V-esima.
    for (const Edge& e : edges) {
        if (dist[e.u] < INF && dist[e.u] + e.w < dist[e.v]) {
            auto b = T.at(__LINE__, __func__);
            b.vars({{"u", g.names[e.u]}, {"v", g.names[e.v]}, {"w", e.w}})
             .graphState(graphSnapshot(e.u, e.v, "rejected"))
             .error("ciclo_negativo")
             .counters({{"rilassamenti", relaxations}})
             .note("CICLO NEGATIVO. Alla passata numero V l'arco " + g.names[e.u] + "->" + g.names[e.v] +
                   " migliora ancora: vuol dire che esiste un giro il cui peso totale è negativo, e "
                   "percorrendolo all'infinito la distanza scende senza fondo. Nessun cammino minimo "
                   "esiste, e l'algoritmo lo segnala invece di restituire numeri privi di senso.");
            emitTables(b);
            b.emit();
            return false;
        }
    }

    {
        auto b = T.at(__LINE__, __func__);
        b.graphState(graphSnapshot(-1, -1, ""))
         .counters({{"rilassamenti", relaxations}})
         .note("La passata di rilevamento non ha migliorato niente: nessun ciclo negativo, le distanze "
               "sono definitive. In verde l'albero dei cammini minimi.");
        emitTables(b);
        b.emit();
    }
    return true;
}

static void run(const trace::GraphData& src, int start) {
    g = src;
    edges.clear();
    for (int i = 0; i < g.size(); i++)
        for (int j = 0; j < g.size(); j++)
            if (g.adj[i][j] != 0) edges.push_back({i, j, g.adj[i][j]});
    relaxations = 0;

    T.input(g.label + ", sorgente " + g.names[start]);
    T.layout(g.layoutJson());
    bellmanFord(start);
}

int main() {
    T.view("graph").complexity("O(V·E)", "O(V)");
    T.panel("table", "Distanze per passata");

    run(trace::negativeEdgeGraph(), 0);
    run(trace::negativeCycleGraph(), 0);
    run(trace::directedGraph(), 0);

    T.dump("traces/bellman_ford.js");
    return 0;
}
