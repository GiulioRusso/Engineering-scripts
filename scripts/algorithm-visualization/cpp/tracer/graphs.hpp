// graphs.hpp - the example graphs, with hand-placed node coordinates.
//
// Small graphs, 5-7 nodes, laid out by hand so they are readable on a projector
// and identical on every reload. Shared by every graph algorithm so that the
// same picture recurs from chapter to chapter and the student can compare what
// BFS, DFS, Dijkstra and Bellman-Ford each do to it.
#pragma once

#include <string>
#include <vector>

#include "tracer.hpp"

namespace trace {

struct GraphData {
    std::string label;
    std::vector<std::string> names;
    std::vector<std::vector<int>> adj;   // 0 = nessun arco, altrimenti il peso
    std::vector<std::pair<int, int>> coords;
    bool directed = false;

    int size() const { return static_cast<int>(names.size()); }

    std::string layoutJson() const {
        std::string s = "{";
        for (size_t i = 0; i < names.size(); ++i) {
            if (i) s += ",";
            s += Val::quote(names[i]) + ":{\"x\":" + std::to_string(coords[i].first) +
                 ",\"y\":" + std::to_string(coords[i].second) + "}";
        }
        return s + "}";
    }
};

// Grafo non orientato e non pesato: quello su cui si confrontano BFS e DFS.
//
//        B ----- D
//      /  \        \
//     A    \        F
//      \    \      /
//        C ----- E
inline GraphData traversalGraph() {
    GraphData g;
    g.label = "grafo non orientato";
    g.names = {"A", "B", "C", "D", "E", "F"};
    g.coords = {{0, 60}, {110, 0}, {110, 120}, {230, 0}, {230, 120}, {340, 60}};
    const int n = 6;
    g.adj.assign(n, std::vector<int>(n, 0));
    auto link = [&](int a, int b) { g.adj[a][b] = 1; g.adj[b][a] = 1; };
    link(0, 1);   // A-B
    link(0, 2);   // A-C
    link(1, 3);   // B-D
    link(1, 4);   // B-E
    link(2, 4);   // C-E
    link(3, 5);   // D-F
    link(4, 5);   // E-F
    return g;
}

// Un secondo grafo, con un ramo che si chiude su se stesso: rende evidente il
// backtracking della DFS.
inline GraphData cycleGraph() {
    GraphData g;
    g.label = "grafo con ciclo";
    g.names = {"A", "B", "C", "D", "E"};
    g.coords = {{0, 0}, {130, -70}, {130, 70}, {260, 0}, {390, 0}};
    const int n = 5;
    g.adj.assign(n, std::vector<int>(n, 0));
    auto link = [&](int a, int b) { g.adj[a][b] = 1; g.adj[b][a] = 1; };
    link(0, 1);
    link(0, 2);
    link(1, 3);
    link(2, 3);
    link(3, 4);
    return g;
}

// Grafo pesato non orientato, per Dijkstra e Kruskal.
inline GraphData weightedGraph() {
    GraphData g;
    g.label = "grafo pesato";
    g.names = {"A", "B", "C", "D", "E", "F"};
    g.coords = {{0, 60}, {120, 0}, {120, 130}, {250, 0}, {250, 130}, {370, 65}};
    const int n = 6;
    g.adj.assign(n, std::vector<int>(n, 0));
    auto link = [&](int a, int b, int w) { g.adj[a][b] = w; g.adj[b][a] = w; };
    link(0, 1, 4);
    link(0, 2, 2);
    link(1, 2, 1);
    link(1, 3, 5);
    link(2, 4, 8);
    link(3, 4, 2);
    link(3, 5, 6);
    link(4, 5, 3);
    return g;
}

// Grafo ORIENTATO e pesato, per Floyd-Warshall.
//
//   A --3--> B      B --2--> C      C --1--> D
//   A --7--> D      B --8--> A      C --5--> A      D --2--> A
inline GraphData directedGraph() {
    GraphData g;
    g.label = "grafo orientato";
    g.names = {"A", "B", "C", "D"};
    g.coords = {{0, 0}, {150, 0}, {150, 130}, {0, 130}};
    g.directed = true;
    const int n = 4;
    g.adj.assign(n, std::vector<int>(n, 0));
    g.adj[0][1] = 3;
    g.adj[0][3] = 7;
    g.adj[1][0] = 8;
    g.adj[1][2] = 2;
    g.adj[2][0] = 5;
    g.adj[2][3] = 1;
    g.adj[3][0] = 2;
    return g;
}

// Grafo orientato con un arco di peso NEGATIVO, ma senza cicli negativi:
// Dijkstra ci sbaglierebbe, Bellman-Ford no.
inline GraphData negativeEdgeGraph() {
    GraphData g;
    g.label = "arco negativo (nessun ciclo negativo)";
    g.names = {"A", "B", "C", "D", "E"};
    g.coords = {{0, 60}, {130, 0}, {130, 130}, {260, 60}, {390, 60}};
    g.directed = true;
    const int n = 5;
    g.adj.assign(n, std::vector<int>(n, 0));
    g.adj[0][1] = 6;
    g.adj[0][2] = 7;
    g.adj[1][2] = 8;
    g.adj[1][3] = 5;
    g.adj[2][3] = -3;
    g.adj[3][4] = 9;
    return g;
}

// Grafo orientato con un CICLO negativo B -> C -> B di peso complessivo -1:
// girandoci dentro la distanza scende senza fondo, e nessun cammino minimo
// esiste. È il caso che la passata di rilevamento deve scoprire.
inline GraphData negativeCycleGraph() {
    GraphData g;
    g.label = "ciclo negativo";
    g.names = {"A", "B", "C", "D"};
    g.coords = {{0, 60}, {140, 0}, {280, 60}, {140, 130}};
    g.directed = true;
    const int n = 4;
    g.adj.assign(n, std::vector<int>(n, 0));
    g.adj[0][1] = 4;
    g.adj[1][2] = 3;
    g.adj[2][3] = 2;
    g.adj[3][1] = -6;   // B -> C -> D -> B costa 3 + 2 - 6 = -1
    return g;
}

}  // namespace trace
