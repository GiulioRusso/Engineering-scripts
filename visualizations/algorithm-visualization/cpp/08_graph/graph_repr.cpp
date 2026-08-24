// Graph Representation: Matrix <-> Adjacency List
//
// The two representations hold the same information at different costs. The
// matrix answers "is there an edge between i and j?" in O(1) but always takes
// V^2 cells, even for a graph with very few edges. The list takes O(V+E) but
// has to walk a list for the same question.
//
// Here the two are filled together, one edge at a time. Hovering an edge of
// the graph jumps the view to the matching step, showing exactly which cells
// and which list entries represent it.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"
#include "../tracer/graphs.hpp"

static Tracer T("graph_repr", "Adjacency: Matrix and List", "Graphs", __FILE__);

static trace::GraphData g;
static std::vector<std::vector<int>> matrix;
static std::vector<std::string> adjList;

// tracer-hide-begin
static std::string matrixSnapshot(int hlRow, int hlCol, int cr, int cc) {
    std::vector<std::vector<std::string>> cells(g.size(), std::vector<std::string>(g.size()));
    for (int i = 0; i < g.size(); i++)
        for (int j = 0; j < g.size(); j++) cells[i][j] = std::to_string(matrix[i][j]);
    return trace::matrixJson(g.names, cells, hlRow, hlCol, cr, cc);
}

static std::string listSnapshot() {
    std::string s = "{";
    for (int i = 0; i < g.size(); i++) {
        if (i) s += ",";
        s += trace::Val::quote(g.names[i]) +
             ":" + trace::Val::quote(adjList[i].empty() ? "-" : adjList[i]);
    }
    return s + "}";
}

static std::string graphSnapshot(int hi, int hj) {
    trace::GraphJson gj;
    for (int i = 0; i < g.size(); i++) gj.node(g.names[i], (i == hi || i == hj) ? "current" : "");
    for (int i = 0; i < g.size(); i++)
        for (int j = i + 1; j < g.size(); j++)
            if (g.adj[i][j])
                gj.edge(g.names[i], g.names[j], "",
                        ((i == hi && j == hj) || (i == hj && j == hi)) ? "current" : "");
    return gj.str();
}
// tracer-hide-end

int main() {
    T.view("graph").complexity("matrix O(V^2) space, list O(V+E)", "—");
    T.panel("matrix", "Adjacency matrix");
    T.panel("table", "Adjacency list");

    for (const trace::GraphData& src : {trace::traversalGraph(), trace::cycleGraph()}) {
        g = src;
        matrix.assign(g.size(), std::vector<int>(g.size(), 0));
        adjList.assign(g.size(), "");

        T.input(g.label);
        T.layout(g.layoutJson());
        T.at(__LINE__, "main")
         .vars({{"V", g.size()}})
         .graphState(graphSnapshot(-1, -1))
         .matrixState(matrixSnapshot(-1, -1, -1, -1))
         .tableRaw("adjacent", listSnapshot())
         .counters({{"edges", 0}})
         .note("Undirected graph with " + std::to_string(g.size()) +
               " nodes. Matrix all zero and lists empty: filling them one edge at a time.")
         .emit();

        int edges = 0;
        for (int i = 0; i < g.size(); i++) {
            for (int j = i + 1; j < g.size(); j++) {
                if (!g.adj[i][j]) continue;

                // An undirected edge lights up TWO cells of the matrix: the
                // symmetry across the diagonal isn't a drawing detail, it's
                // exactly what "undirected" means.
                matrix[i][j] = 1;
                matrix[j][i] = 1;
                adjList[i] += (adjList[i].empty() ? "" : ", ") + g.names[j];
                adjList[j] += (adjList[j].empty() ? "" : ", ") + g.names[i];
                edges++;

                T.at(__LINE__, "main")
                 .vars({{"i", i}, {"j", j}})
                 .graphState(graphSnapshot(i, j))
                 .matrixState(matrixSnapshot(i, j, i, j))
                 .tableRaw("adjacent", listSnapshot())
                 .link(g.names[i], g.names[j])
                 .counters({{"edges", edges}})
                 .note("Edge " + g.names[i] + "-" + g.names[j] + ": lighting up cells [" + g.names[i] +
                       "][" + g.names[j] + "] and [" + g.names[j] + "][" + g.names[i] +
                       "], and adding each to the other's list.")
                 .emit();
            }
        }

        T.at(__LINE__, "main")
         .vars({{"V", g.size()}, {"E", edges}, {"matrix cells", g.size() * g.size()}})
         .graphState(graphSnapshot(-1, -1))
         .matrixState(matrixSnapshot(-1, -1, -1, -1))
         .tableRaw("adjacent", listSnapshot())
         .counters({{"edges", edges}})
         .note("Done. The matrix is symmetric and takes " + std::to_string(g.size() * g.size()) +
               " cells for just " + std::to_string(edges) +
               " edges: on a sparse graph that's almost all waste, which is why the list is used instead.")
         .emit();
    }

    T.dump("traces/graph_repr.js");
    return 0;
}
