// Kruskal
//
// Minimum spanning tree. Greedy strategy in its purest form: sort all the
// edges by weight and take them one at a time, lightest first, skipping any
// that would close a cycle. It stops at V-1 edges, exactly as many as are
// needed to connect V nodes with no cycles.
//
// The question "does this edge close a cycle?" becomes "are the two
// endpoints already in the same component?", answered by Union-Find: findSet
// walks up to the component's representative, unionSets merges two
// components by attaching the shorter tree under the taller one (union by
// rank), so the chains stay short.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"
#include "../tracer/graphs.hpp"

static Tracer T("kruskal", "Kruskal", "Flow and MST", __FILE__);

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
        else value = isAccepted(e) ? "OK" : (isRejected(e) ? "cycle" : "-");
        s += trace::Val::quote(std::to_string(e)) + ":" + trace::Val::quote(value);
    }
    return s + "}";
}

static void emitTables(trace::StepBuilder& b) {
    b.tableRaw("parent", nodeTable(parent, true))
     .tableRaw("rank", nodeTable(rankArr, false))
     .tableRaw("edge", edgeTable(0))
     .tableRaw("weight", edgeTable(1))
     .tableRaw("status", edgeTable(2));
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
         .counters({{"accepted", 0}, {"total weight", 0}})
         .note("Edges sorted by increasing weight. Every node is its own component: parent[x] = x and "
               "rank 0 for all. " + std::to_string(g.size() - 1) + " edges are needed.");
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
            b.vars({{"i", (int)i}, {"edge", g.names[u] + "-" + g.names[v]}, {"weight", edges[i].w},
                    {"findSet(u)", g.names[ru]}, {"findSet(v)", g.names[rv]}})
             .marks({{"cursor", (int)i}})
             .graphState(graphSnapshot(i))
             .counters({{"accepted", (int)accepted.size()}, {"total weight", totalWeight}})
             .note("Edge " + g.names[u] + "-" + g.names[v] + " of weight " + std::to_string(edges[i].w) +
                   ". findSet says: " + g.names[u] + " is in component " + g.names[ru] + ", " +
                   g.names[v] + " in component " + g.names[rv] + ".");
            emitTables(b);
            b.emit();
        }

        if (ru == rv) {
            rejected.push_back(i);
            auto b = T.at(__LINE__, __func__);
            b.vars({{"i", (int)i}, {"edge", g.names[u] + "-" + g.names[v]}})
             .marks({{"cursor", (int)i}})
             .graphState(graphSnapshot(i))
             .reject(g.names[u] + "-" + g.names[v])
             .counters({{"accepted", (int)accepted.size()}, {"total weight", totalWeight}})
             .note("REJECTED: same component, so the two endpoints are already connected and this "
                   "edge would close a cycle. A discarded edge is as informative as an accepted one.");
            emitTables(b);
            b.emit();
            continue;
        }

        unionSets(u, v);
        accepted.push_back(i);
        totalWeight += edges[i].w;
        {
            auto b = T.at(__LINE__, __func__);
            b.vars({{"i", (int)i}, {"edge", g.names[u] + "-" + g.names[v]},
                    {"total weight", totalWeight}})
             .marks({{"cursor", (int)i}})
             .graphState(graphSnapshot(i))
             .accept(g.names[u] + "-" + g.names[v])
             .counters({{"accepted", (int)accepted.size()}, {"total weight", totalWeight}})
             .note("ACCEPTED: different components. unionSets merges them by attaching the "
                   "lower-rank root under the higher-rank one — watch how parent[] changes.");
            emitTables(b);
            b.emit();
        }

        if ((int)accepted.size() == g.size() - 1) {
            auto b = T.at(__LINE__, __func__);
            b.vars({{"edges", (int)accepted.size()}, {"total weight", totalWeight}})
             .graphState(graphSnapshot(-1))
             .counters({{"accepted", (int)accepted.size()}, {"total weight", totalWeight}})
             .note("V-1 = " + std::to_string(g.size() - 1) + " edges taken: the spanning tree is "
                   "complete, total weight " + std::to_string(totalWeight) +
                   ". Every remaining edge would close a cycle, so we can stop right here.");
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
    T.clearWatches().watchArray("weights", weights, edgeCount);
    // The sorted weights, for the panel with the advancing cursor.
    std::vector<Edge> sorted = edges;
    std::sort(sorted.begin(), sorted.end(), [](const Edge& a, const Edge& b) { return a.w < b.w; });
    for (size_t i = 0; i < sorted.size(); i++) weights[i] = sorted[i].w;

    kruskal();
}

int main() {
    T.view("graph").complexity("O(E log E)", "O(V)");
    T.panel("array", "Edges sorted by weight", "weights", "cursor");
    T.panel("table", "Union-Find: parent[] and rank[]", "parent,rank");
    T.panel("table", "Edges", "edge,weight,status");

    run(trace::weightedGraph());

    T.dump("traces/kruskal.js");
    return 0;
}
