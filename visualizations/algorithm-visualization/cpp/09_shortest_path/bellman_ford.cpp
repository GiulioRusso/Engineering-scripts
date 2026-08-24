// Bellman-Ford
//
// Shortest paths from one source, and unlike Dijkstra it also works with
// negative edge weights. In exchange it's slower: instead of picking the best
// node each time, it relaxes EVERY edge, and does so V-1 times.
//
// Why exactly V-1? A simple shortest path touches at most V nodes, so it has
// at most V-1 edges. After pass number p, every distance reachable with p
// edges is correct; after V-1 passes, all of them are.
//
// If a V-th pass can still improve something, then there's a path with V
// edges or more that keeps getting shorter: a NEGATIVE CYCLE, and the
// shortest path simply doesn't exist.
#include <climits>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"
#include "../tracer/graphs.hpp"

static Tracer T("bellman_ford", "Bellman-Ford", "Shortest Paths", __FILE__);

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
// The book's table: one row per pass, one column per vertex.
static void emitTables(trace::StepBuilder& b) {
    for (const auto& row : history) b.tableRaw(row.first, row.second);
}
// tracer-hide-end

// The book's table: one row per pass, one column per vertex.

bool bellmanFord(int src) {
    for (int i = 0; i < g.size(); i++) {
        dist[i] = INF;
        parent[i] = -1;
    }
    dist[src] = 0;
    history.clear();
    history.push_back({"initial", rowJson()});
    {
        auto b = T.at(__LINE__, __func__);
        b.vars({{"source", g.names[src]}, {"V", g.size()}, {"E", (int)edges.size()}})
         .graphState(graphSnapshot(-1, -1, ""))
         .counters({{"relaxations", relaxations}})
         .note("Distances at infinity except the source. It will take " + std::to_string(g.size() - 1) +
               " full passes over all " + std::to_string(edges.size()) + " edges.");
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
                b.vars({{"pass", pass}, {"u", g.names[e.u]}, {"v", g.names[e.v]}, {"w", e.w}})
                 .graphState(graphSnapshot(e.u, e.v, "current"))
                 .relax(g.names[e.u], g.names[e.v], dist[e.v])
                 .counters({{"relaxations", relaxations}})
                 .note("Pass " + std::to_string(pass) + ", edge " + g.names[e.u] + "->" +
                       g.names[e.v] + " (weight " + std::to_string(e.w) + "): " + distText(e.u) + " + " +
                       std::to_string(e.w) + " = " + distText(e.v) + ", better than " +
                       (before >= INF ? std::string("inf") : std::to_string(before)) + ".");
                emitTables(b);
                b.emit();
            } else {
                auto b = T.at(__LINE__, __func__);
                b.vars({{"pass", pass}, {"u", g.names[e.u]}, {"v", g.names[e.v]}, {"w", e.w}})
                 .graphState(graphSnapshot(e.u, e.v, "rejected"))
                 .counters({{"relaxations", relaxations}})
                 .note("Edge " + g.names[e.u] + "->" + g.names[e.v] +
                       ": no improvement. Bellman-Ford looks at every edge on every pass, even "
                       "the ones that will never help.");
                emitTables(b);
                b.emit();
            }
        }

        history.push_back({"pass " + std::to_string(pass), rowJson()});
        {
            auto b = T.at(__LINE__, __func__);
            b.vars({{"pass", pass}, {"changed", changed}})
             .graphState(graphSnapshot(-1, -1, ""))
             .counters({{"relaxations", relaxations}})
             .note("End of pass " + std::to_string(pass) + ". Every distance reachable with at most " +
                   std::to_string(pass) + " edges is now correct." +
                   (changed ? "" : " No change: later passes won't change anything."));
            emitTables(b);
            b.emit();
        }
    }

    // Detection pass: the V-th one.
    for (const Edge& e : edges) {
        if (dist[e.u] < INF && dist[e.u] + e.w < dist[e.v]) {
            auto b = T.at(__LINE__, __func__);
            b.vars({{"u", g.names[e.u]}, {"v", g.names[e.v]}, {"w", e.w}})
             .graphState(graphSnapshot(e.u, e.v, "rejected"))
             .error("ciclo_negativo")  // matched literally by tools/check_traces.js; do not rename
             .counters({{"relaxations", relaxations}})
             .note("NEGATIVE CYCLE. On pass number V the edge " + g.names[e.u] + "->" + g.names[e.v] +
                   " still improves: that means there's a loop whose total weight is negative, and "
                   "going around it forever the distance drops without a floor. No shortest path "
                   "exists, and the algorithm flags it instead of returning meaningless numbers.");
            emitTables(b);
            b.emit();
            return false;
        }
    }

    {
        auto b = T.at(__LINE__, __func__);
        b.graphState(graphSnapshot(-1, -1, ""))
         .counters({{"relaxations", relaxations}})
         .note("The detection pass improved nothing: no negative cycle, the distances are final. "
               "The shortest-path tree is highlighted in green.");
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

    T.input(g.label + ", source " + g.names[start]);
    T.layout(g.layoutJson());
    bellmanFord(start);
}

int main() {
    T.view("graph").complexity("O(V*E)", "O(V)");
    T.panel("table", "Distances per pass");

    run(trace::negativeEdgeGraph(), 0);
    run(trace::negativeCycleGraph(), 0);
    run(trace::directedGraph(), 0);

    T.dump("traces/bellman_ford.js");
    return 0;
}
