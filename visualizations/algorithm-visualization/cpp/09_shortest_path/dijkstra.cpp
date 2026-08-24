// Dijkstra
//
// Shortest paths from one source, over non-negative edge weights. The idea:
// pull out the not-yet-visited node with the smallest provisional distance,
// declare it final, and try to improve its neighbours. The extracted node is
// final because any other path would have to go through a node whose
// distance is already larger, and non-negative weights can't make it shrink
// from there. This is exactly the point that breaks with a negative edge.
//
// The book's code extracts the minimum with a linear scan, not a priority
// queue: hence the O(V^2) cost. That's the version implemented here, even
// though the book's prose talks about a priority queue.
#include <climits>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"
#include "../tracer/graphs.hpp"

static Tracer T("dijkstra", "Dijkstra", "Shortest Paths", __FILE__);

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
        s += trace::Val::quote(g.names[i]) + ":" + trace::Val::quote(visited[i] ? "yes" : "-");
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

// Extracting the minimum by linear scan: O(V) every time, and that's what
// brings the total to O(V^2).
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
     .vars({{"source", g.names[src]}})
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("dist", distTable())
     .tableRaw("visited", visitedTable())
     .tableRaw("parent", parentTable())
     .counters({{"relaxations", relaxations}})
     .note("Every distance starts at infinity except the source, which is 0. No node is final yet.")
     .emit();

    for (int step = 0; step < g.size(); step++) {
        int u = minDistance();
        if (u < 0 || dist[u] == INT_MAX) break;

        T.at(__LINE__, __func__)
         .vars({{"u", g.names[u]}, {"dist[u]", distText(u)}})
         .graphState(graphSnapshot(u, -1))
         .tableRaw("dist", distTable())
         .tableRaw("visited", visitedTable())
         .tableRaw("parent", parentTable())
         .counters({{"relaxations", relaxations}})
         .note("Extracting the unvisited node with minimum distance: " + g.names[u] + " at " + distText(u) +
               ". A linear scan over all V nodes, not a priority queue.")
         .emit();

        visited[u] = true;
        T.at(__LINE__, __func__)
         .vars({{"u", g.names[u]}, {"dist[u]", distText(u)}})
         .graphState(graphSnapshot(u, -1))
         .tableRaw("dist", distTable())
         .tableRaw("visited", visitedTable())
         .tableRaw("parent", parentTable())
         .visit(g.names[u])
         .counters({{"relaxations", relaxations}})
         .note("Declaring " + g.names[u] + " final. No path can shorten it any further: any "
               "alternative would have to go through a node already farther away, and with "
               "non-negative weights it can only grow from there.")
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
                 .counters({{"relaxations", relaxations}})
                 .note("RELAXATION: dist[" + g.names[u] + "] + " + std::to_string(w) + " = " +
                       distText(v) + " < " +
                       (before == INT_MAX ? std::string("inf") : std::to_string(before)) +
                       ". Updating dist[" + g.names[v] + "] and marking " + g.names[u] + " as its predecessor.")
                 .emit();
            } else {
                T.at(__LINE__, __func__)
                 .vars({{"u", g.names[u]}, {"v", g.names[v]}, {"w", w}})
                 .graphState(graphSnapshot(u, v))
                 .tableRaw("dist", distTable())
                 .tableRaw("visited", visitedTable())
                 .tableRaw("parent", parentTable())
                 .counters({{"relaxations", relaxations}})
                 .note("Going through " + g.names[u] + " would cost " +
                       std::to_string(dist[u] + w) + ", not less than " + distText(v) +
                       ": no improvement, dist[" + g.names[v] + "] stays as it is.")
                 .emit();
            }
        }
    }

    T.at(__LINE__, __func__)
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("dist", distTable())
     .tableRaw("visited", visitedTable())
     .tableRaw("parent", parentTable())
     .counters({{"relaxations", relaxations}})
     .note("Done. dist[] holds the shortest distances from the source and parent[] the "
           "shortest-path tree, highlighted in green on the graph.")
     .emit();
}

static void run(const trace::GraphData& src, int start) {
    g = src;
    relaxations = 0;
    T.input(g.label + ", source " + g.names[start]);
    T.layout(g.layoutJson());
    dijkstra(start);
}

int main() {
    T.view("graph").complexity("O(V^2) with a linear scan", "O(V)");
    T.panel("table", "dist[], visited[] and parent[]");

    run(trace::weightedGraph(), 0);
    run(trace::weightedGraph(), 5);

    T.dump("traces/dijkstra.js");
    return 0;
}
