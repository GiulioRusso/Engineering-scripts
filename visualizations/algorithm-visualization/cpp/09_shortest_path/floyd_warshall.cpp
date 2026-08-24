// Floyd-Warshall
//
// Shortest paths between EVERY pair, not from a single source. The content of
// the algorithm is the evolving matrix, not the graph: that's why the primary
// view here is the matrix and the graph is a side panel.
//
// The whole idea is in the meaning of the outer loop: after pass k, cell
// [i][j] holds the shortest path from i to j that can use only the first k+1
// vertices as intermediate nodes. The passes produce the sequence
// A^0, A^1, A^2, ... and the last one is the answer.
//
//   dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j])
#include <climits>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"
#include "../tracer/graphs.hpp"

static Tracer T("floyd_warshall", "Floyd-Warshall", "Shortest Paths", __FILE__);

static const int INF = 1000000;
static trace::GraphData g;
static std::vector<std::vector<int>> dist;
static int updates = 0;

// tracer-hide-begin
static std::string cellText(int v) { return v >= INF ? "inf" : std::to_string(v); }

static std::string sup(int k) {
    const char* digits[] = {"\u2070", "\u00b9", "\u00b2", "\u00b3", "\u2074",
                            "\u2075", "\u2076", "\u2077", "\u2078", "\u2079"};
    return k >= 0 && k < 10 ? digits[k] : std::to_string(k);
}

// The vertices allowed as intermediates after pass k.
static std::string allowed(int k) {
    std::string s;
    for (int t = 0; t <= k; t++) s += (t ? ", " : "") + g.names[t];
    return s;
}

static std::string matrixSnapshot(int hlRow, int hlCol, int cr, int cc) {
    std::vector<std::vector<std::string>> cells(g.size(), std::vector<std::string>(g.size()));
    for (int i = 0; i < g.size(); i++)
        for (int j = 0; j < g.size(); j++) cells[i][j] = cellText(dist[i][j]);
    return trace::matrixJson(g.names, cells, hlRow, hlCol, cr, cc);
}

static std::string graphSnapshot(int k, int i, int j) {
    trace::GraphJson gj;
    for (int v = 0; v < g.size(); v++) {
        const char* st = "";
        if (v == k) st = "current";
        else if (v == i || v == j) st = "frontier";
        gj.node(g.names[v], st);
    }
    for (int a = 0; a < g.size(); a++)
        for (int b = 0; b < g.size(); b++)
            if (g.adj[a][b]) {
                bool hot = (a == i && b == k) || (a == k && b == j);
                gj.edge(g.names[a], g.names[b], std::to_string(g.adj[a][b]),
                        hot ? "current" : "", true);
            }
    return gj.str();
}
// tracer-hide-end

void floydWarshall() {
    for (int i = 0; i < g.size(); i++) {
        for (int j = 0; j < g.size(); j++) {
            if (i == j) dist[i][j] = 0;
            else if (g.adj[i][j]) dist[i][j] = g.adj[i][j];
            else dist[i][j] = INF;
        }
    }
    T.at(__LINE__, __func__)
     .vars({{"V", g.size()}})
     .matrixState(matrixSnapshot(-1, -1, -1, -1))
     .graphState(graphSnapshot(-1, -1, -1))
     .counters({{"updates", updates}})
     .note("A^0: the initial matrix. Diagonal at 0, the edge weight where an edge exists, infinity "
           "everywhere else. No intermediate node is allowed yet.")
     .emit();

    for (int k = 0; k < g.size(); k++) {
        T.at(__LINE__, __func__)
         .vars({{"k", g.names[k]}})
         .matrixState(matrixSnapshot(k, k, -1, -1))
         .graphState(graphSnapshot(k, -1, -1))
         .counters({{"updates", updates}})
         .note("Pass k = " + g.names[k] + ". From here on " + g.names[k] +
               " is allowed as an intermediate node. Row " + g.names[k] + " and column " + g.names[k] +
               " are the only two ingredients of every comparison in this pass.")
         .emit();

        for (int i = 0; i < g.size(); i++) {
            for (int j = 0; j < g.size(); j++) {
                if (dist[i][k] + dist[k][j] < dist[i][j]) {
                    int before = dist[i][j];
                    dist[i][j] = dist[i][k] + dist[k][j];
                    updates++;
                    T.at(__LINE__, __func__)
                     .vars({{"k", g.names[k]}, {"i", g.names[i]}, {"j", g.names[j]},
                            {"dist[i][k]", cellText(dist[i][k])}, {"dist[k][j]", cellText(dist[k][j])}})
                     .matrixState(matrixSnapshot(k, k, i, j))
                     .graphState(graphSnapshot(k, i, j))
                     .relax(g.names[i], g.names[j], dist[i][j])
                     .counters({{"updates", updates}})
                     .note("Going through " + g.names[k] + ": " + cellText(dist[i][k]) + " + " +
                           cellText(dist[k][j]) + " = " + cellText(dist[i][j]) + ", better than " +
                           cellText(before) + ". Updating [" + g.names[i] + "][" + g.names[j] + "].")
                     .emit();
                }
            }
        }

        T.at(__LINE__, __func__)
         .vars({{"k", g.names[k]}})
         .matrixState(matrixSnapshot(-1, -1, -1, -1))
         .graphState(graphSnapshot(-1, -1, -1))
         .counters({{"updates", updates}})
         .note("A" + sup(k + 1) + ": every cell is the shortest path that can pass only through {" +
               allowed(k) + "} as intermediate nodes.")
         .emit();
    }

    T.at(__LINE__, __func__)
     .matrixState(matrixSnapshot(-1, -1, -1, -1))
     .graphState(graphSnapshot(-1, -1, -1))
     .counters({{"updates", updates}})
     .note("Final matrix: the shortest distance between every pair of vertices, in O(V^3) time and "
           "O(V^2) space, with three nested loops and no auxiliary structure.")
     .emit();
}

static void run(const trace::GraphData& src) {
    g = src;
    dist.assign(g.size(), std::vector<int>(g.size(), INF));
    updates = 0;
    T.input(g.label);
    T.layout(g.layoutJson());
    floydWarshall();
}

int main() {
    T.view("matrix").complexity("O(V^3)", "O(V^2)");
    T.panel("graph", "Graph");

    run(trace::directedGraph());
    run(trace::negativeEdgeGraph());

    T.dump("traces/floyd_warshall.js");
    return 0;
}
