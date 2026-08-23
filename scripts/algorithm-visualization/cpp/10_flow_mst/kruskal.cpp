// Kruskal
//
// Albero ricoprente minimo. Strategia golosa nella sua forma più pura: si
// ordinano tutti gli archi per peso e si prendono uno alla volta, dal più
// leggero, saltando quelli che chiuderebbero un ciclo. Ci si ferma a V-1 archi,
// che è esattamente quanti ne servono per collegare V nodi senza cicli.
//
// La domanda "questo arco chiude un ciclo?" diventa "i due estremi stanno già
// nella stessa componente?", e a quella risponde la Union-Find: findSet risale
// al rappresentante della componente, unionSets fonde due componenti attaccando
// l'albero più basso sotto quello più alto (l'unione per rango), così le catene
// restano corte.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"
#include "../tracer/graphs.hpp"

static Tracer T("kruskal", "Kruskal", "Flusso e MST", __FILE__);

struct Edge {
    int u, v, w;
};

static trace::GraphData g;
static std::vector<Edge> edges;
static std::vector<int> parent, rankArr;
static std::vector<int> accepted, rejected;
static int weights[16];
static int edgeCount = 0;
static int totalWeight = 0;

int findSet(int x) {
    while (parent[x] != x) x = parent[x];
    return x;
}

void unionSets(int a, int b) {
    int ra = findSet(a);
    int rb = findSet(b);
    if (rankArr[ra] < rankArr[rb]) {
        parent[ra] = rb;
    } else if (rankArr[ra] > rankArr[rb]) {
        parent[rb] = ra;
    } else {
        parent[rb] = ra;
        rankArr[ra]++;
    }
}

// tracer-hide-begin
static bool isAccepted(int i) {
    for (int a : accepted) if (a == i) return true;
    return false;
}
static bool isRejected(int i) {
    for (int r : rejected) if (r == i) return true;
    return false;
}

static std::string graphSnapshot(int cursor) {
    trace::GraphJson gj;
    for (int i = 0; i < g.size(); i++) {
        const char* st = "";
        if (cursor >= 0 && (edges[cursor].u == i || edges[cursor].v == i)) st = "current";
        gj.node(g.names[i], st, "comp " + g.names[findSet(i)]);
    }
    for (size_t e = 0; e < edges.size(); e++) {
        const char* st = "";
        if ((int)e == cursor) st = "current";
        else if (isAccepted(e)) st = "accepted";
        else if (isRejected(e)) st = "rejected";
        gj.edge(g.names[edges[e].u], g.names[edges[e].v], std::to_string(edges[e].w), st);
    }
    return gj.str();
}

static std::string nodeTable(const std::vector<int>& arr, bool asName) {
    std::string s = "{";
    for (int i = 0; i < g.size(); i++) {
        if (i) s += ",";
        s += trace::Val::quote(g.names[i]) + ":" +
             trace::Val::quote(asName ? g.names[arr[i]] : std::to_string(arr[i]));
    }
    return s + "}";
}

static std::string edgeTable(int which) {
    std::string s = "{";
    for (size_t e = 0; e < edges.size(); e++) {
        if (e) s += ",";
        std::string value;
        if (which == 0) value = g.names[edges[e].u] + "-" + g.names[edges[e].v];
        else if (which == 1) value = std::to_string(edges[e].w);
        else value = isAccepted(e) ? "OK" : (isRejected(e) ? "ciclo" : "-");
        s += trace::Val::quote(std::to_string(e)) + ":" + trace::Val::quote(value);
    }
    return s + "}";
}

static void emitTables(trace::StepBuilder& b) {
    b.tableRaw("parent", nodeTable(parent, true))
     .tableRaw("rank", nodeTable(rankArr, false))
     .tableRaw("arco", edgeTable(0))
     .tableRaw("peso", edgeTable(1))
     .tableRaw("stato", edgeTable(2));
}
// tracer-hide-end

void kruskal() {
    parent.resize(g.size());
    rankArr.assign(g.size(), 0);
    for (int i = 0; i < g.size(); i++) parent[i] = i;

    std::sort(edges.begin(), edges.end(),
              [](const Edge& a, const Edge& b) { return a.w < b.w; });
    {
        auto b = T.at(__LINE__, __func__);
        b.vars({{"V", g.size()}, {"E", (int)edges.size()}})
         .graphState(graphSnapshot(-1))
         .counters({{"accettati", 0}, {"peso totale", 0}})
         .note("Archi ordinati per peso crescente. Ogni nodo è la propria componente: parent[x] = x e "
               "rank 0 per tutti. Servono " + std::to_string(g.size() - 1) + " archi.");
        emitTables(b);
        b.emit();
    }

    for (size_t i = 0; i < edges.size(); i++) {
        int u = edges[i].u;
        int v = edges[i].v;
        int ru = findSet(u);
        int rv = findSet(v);
        {
            auto b = T.at(__LINE__, __func__);
            b.vars({{"i", (int)i}, {"arco", g.names[u] + "-" + g.names[v]}, {"peso", edges[i].w},
                    {"findSet(u)", g.names[ru]}, {"findSet(v)", g.names[rv]}})
             .marks({{"cursore", (int)i}})
             .graphState(graphSnapshot(i))
             .counters({{"accettati", (int)accepted.size()}, {"peso totale", totalWeight}})
             .note("Arco " + g.names[u] + "-" + g.names[v] + " di peso " + std::to_string(edges[i].w) +
                   ". findSet dice: " + g.names[u] + " sta nella componente " + g.names[ru] + ", " +
                   g.names[v] + " in quella " + g.names[rv] + ".");
            emitTables(b);
            b.emit();
        }

        if (ru == rv) {
            rejected.push_back(i);
            auto b = T.at(__LINE__, __func__);
            b.vars({{"i", (int)i}, {"arco", g.names[u] + "-" + g.names[v]}})
             .marks({{"cursore", (int)i}})
             .graphState(graphSnapshot(i))
             .reject(g.names[u] + "-" + g.names[v])
             .counters({{"accettati", (int)accepted.size()}, {"peso totale", totalWeight}})
             .note("RIFIUTATO: stessa componente, quindi i due estremi sono già collegati e questo "
                   "arco chiuderebbe un ciclo. Un arco scartato è informativo quanto uno preso.");
            emitTables(b);
            b.emit();
            continue;
        }

        unionSets(u, v);
        accepted.push_back(i);
        totalWeight += edges[i].w;
        {
            auto b = T.at(__LINE__, __func__);
            b.vars({{"i", (int)i}, {"arco", g.names[u] + "-" + g.names[v]},
                    {"peso totale", totalWeight}})
             .marks({{"cursore", (int)i}})
             .graphState(graphSnapshot(i))
             .accept(g.names[u] + "-" + g.names[v])
             .counters({{"accettati", (int)accepted.size()}, {"peso totale", totalWeight}})
             .note("ACCETTATO: componenti diverse. unionSets le fonde attaccando la radice di rango "
                   "minore sotto quella di rango maggiore — guarda come cambia parent[].");
            emitTables(b);
            b.emit();
        }

        if ((int)accepted.size() == g.size() - 1) {
            auto b = T.at(__LINE__, __func__);
            b.vars({{"archi", (int)accepted.size()}, {"peso totale", totalWeight}})
             .graphState(graphSnapshot(-1))
             .counters({{"accettati", (int)accepted.size()}, {"peso totale", totalWeight}})
             .note("V-1 = " + std::to_string(g.size() - 1) + " archi presi: l'albero ricoprente è "
                   "completo, peso totale " + std::to_string(totalWeight) +
                   ". Gli archi rimanenti chiuderebbero tutti un ciclo, quindi si può uscire subito.");
            emitTables(b);
            b.emit();
            return;
        }
    }
}

static void run(const trace::GraphData& src) {
    g = src;
    edges.clear();
    accepted.clear();
    rejected.clear();
    totalWeight = 0;
    for (int i = 0; i < g.size(); i++)
        for (int j = i + 1; j < g.size(); j++)
            if (g.adj[i][j]) edges.push_back({i, j, g.adj[i][j]});

    T.input(g.label);
    T.layout(g.layoutJson());
    edgeCount = static_cast<int>(edges.size());
    T.clearWatches().watchArray("pesi", weights, edgeCount);
    // I pesi ordinati, per il pannello con il cursore che avanza.
    std::vector<Edge> sorted = edges;
    std::sort(sorted.begin(), sorted.end(), [](const Edge& a, const Edge& b) { return a.w < b.w; });
    for (size_t i = 0; i < sorted.size(); i++) weights[i] = sorted[i].w;

    kruskal();
}

int main() {
    T.view("graph").complexity("O(E log E)", "O(V)");
    T.panel("array", "Archi ordinati per peso", "pesi", "cursore");
    T.panel("table", "Union-Find: parent[] e rank[]", "parent,rank");
    T.panel("table", "Archi", "arco,peso,stato");

    run(trace::weightedGraph());

    T.dump("traces/kruskal.js");
    return 0;
}
