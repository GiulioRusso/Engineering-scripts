// Ford-Fulkerson
//
// Maximum flow from a source to a sink. The idea is simple: as long as
// there's a path from source to sink in the RESIDUAL graph, push as much
// flow through it as fits — the bottleneck Δ, i.e. the minimum residual
// capacity along the path — and repeat.
//
// The part everyone gets wrong is the BACKWARD edge. When flow is sent from
// u to v, capacity from v to u is added in the residual graph. That
// backward edge isn't a bookkeeping trick: using it means DECREASING the
// flow on u->v, i.e. undoing an earlier choice. Without that possibility
// the algorithm would stay stuck with a first wrong decision.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("ford_fulkerson", "Ford-Fulkerson", "Flow and MST", __FILE__);

static const int N = 6;
static const char* names[N] = {"S", "A", "B", "C", "D", "T"};
static const int coordX[N] = {0, 130, 270, 130, 270, 400};
static const int coordY[N] = {70, 0, 0, 150, 150, 75};

static int capacity[N][N];
static int residual[N][N];
static int maxFlow = 0;
static int augmentations = 0;

// tracer-hide-begin
static std::string layoutJson() {
    std::string s = "{";
    for (int i = 0; i < N; i++) {
        if (i) s += ",";
        s += trace::Val::quote(names[i]) + ":{\"x\":" + std::to_string(coordX[i]) +
             ",\"y\":" + std::to_string(coordY[i]) + "}";
    }
    return s + "}";
}

static bool onPath(const std::vector<int>& path, int u, int v) {
    for (size_t k = 1; k < path.size(); k++)
        if (path[k - 1] == u && path[k] == v) return true;
    return false;
}

// Flow graph: flow/capacity labels, like in the book.
static std::string flowGraph(const std::vector<int>& path, int cur) {
    trace::GraphJson gj;
    for (int i = 0; i < N; i++) {
        bool inPath = false;
        for (int p : path) if (p == i) inPath = true;
        gj.node(names[i], i == cur ? "current" : (inPath ? "path" : ""));
    }
    for (int u = 0; u < N; u++)
        for (int v = 0; v < N; v++)
            if (capacity[u][v] > 0) {
                int flow = capacity[u][v] - residual[u][v];
                gj.edge(names[u], names[v],
                        std::to_string(flow) + "/" + std::to_string(capacity[u][v]),
                        onPath(path, u, v) ? "current" : (flow > 0 ? "accepted" : ""), true);
            }
    return gj.str();
}

// Residual graph: backward edges only show up here, which is why it's worth
// looking at.
static std::string residualGraph(const std::vector<int>& path) {
    trace::GraphJson gj;
    for (int i = 0; i < N; i++) {
        bool inPath = false;
        for (int p : path) if (p == i) inPath = true;
        gj.node(names[i], inPath ? "path" : "");
    }
    for (int u = 0; u < N; u++)
        for (int v = 0; v < N; v++)
            if (residual[u][v] > 0) {
                bool backward = capacity[u][v] == 0;
                gj.edge(names[u], names[v], std::to_string(residual[u][v]),
                        onPath(path, u, v) ? "current" : (backward ? "rejected" : ""), true);
            }
    return gj.str();
}

static std::string residualMatrix(int hr, int hc) {
    std::vector<std::string> labels(names, names + N);
    std::vector<std::vector<std::string>> cells(N, std::vector<std::string>(N));
    for (int u = 0; u < N; u++)
        for (int v = 0; v < N; v++)
            cells[u][v] = residual[u][v] ? std::to_string(residual[u][v]) : "-";
    return trace::matrixJson(labels, cells, hr, hc, hr, hc);
}

static std::string pathText(const std::vector<int>& path) {
    std::string s;
    for (size_t i = 0; i < path.size(); i++) s += (i ? " -> " : "") + std::string(names[path[i]]);
    return s;
}
// tracer-hide-end

// DFS in the residual graph: any path works, it doesn't need to be the shortest.
bool findPath(int u, int sink, std::vector<bool>& visited, std::vector<int>& path) {
    visited[u] = true;
    path.push_back(u);
    T.at(__LINE__, __func__)
     .vars({{"u", names[u]}})
     .graphState(flowGraph(path, u))
     .graphState("residual", residualGraph(path))
     .matrixState(residualMatrix(u, -1))
     .counters({{"paths", augmentations}, {"flow", maxFlow}})
     .note("DFS in the residual graph, currently at " + std::string(names[u]) +
           ". Looking for an edge with residual capacity still available.")
     .emit();

    if (u == sink) return true;

    for (int v = 0; v < N; v++) {
        if (residual[u][v] <= 0 || visited[v]) continue;
        if (findPath(v, sink, visited, path)) return true;
    }

    path.pop_back();
    T.at(__LINE__, __func__)
     .vars({{"u", names[u]}})
     .graphState(flowGraph(path, u))
     .graphState("residual", residualGraph(path))
     .matrixState(residualMatrix(u, -1))
     .counters({{"paths", augmentations}, {"flow", maxFlow}})
     .note("No way forward from " + std::string(names[u]) + ": backtracking.")
     .emit();
    return false;
}

int fordFulkerson(int source, int sink) {
    for (int u = 0; u < N; u++)
        for (int v = 0; v < N; v++) residual[u][v] = capacity[u][v];

    T.at(__LINE__, __func__)
     .vars({{"flow", maxFlow}})
     .graphState(flowGraph({}, -1))
     .graphState("residual", residualGraph({}))
     .matrixState(residualMatrix(-1, -1))
     .counters({{"paths", 0}, {"flow", 0}})
     .note("Zero flow everywhere: the residual graph matches the capacity. Labels are flow/capacity.")
     .emit();

    while (true) {
        std::vector<bool> visited(N, false);
        std::vector<int> path;
        if (!findPath(source, sink, visited, path)) {
            T.at(__LINE__, __func__)
             .vars({{"max flow", maxFlow}})
             .graphState(flowGraph({}, -1))
             .graphState("residual", residualGraph({}))
             .matrixState(residualMatrix(-1, -1))
             .counters({{"paths", augmentations}, {"flow", maxFlow}})
             .note("No augmenting path: the maximum flow is " + std::to_string(maxFlow) +
                   ". The nodes still reachable from the source in the residual graph form the "
                   "minimum cut, and its capacity is exactly this number.")
             .emit();
            break;
        }

        int delta = 1000000;
        for (size_t k = 1; k < path.size(); k++)
            delta = std::min(delta, residual[path[k - 1]][path[k]]);

        augmentations++;
        T.at(__LINE__, __func__)
         .vars({{"path", pathText(path)}, {"delta", delta}})
         .graphState(flowGraph(path, -1))
         .graphState("residual", residualGraph(path))
         .matrixState(residualMatrix(-1, -1))
         .counters({{"paths", augmentations}, {"flow", maxFlow}})
         .note("Augmenting path " + pathText(path) + ". The bottleneck is Δ = " +
               std::to_string(delta) + ": it's the smallest residual capacity along the path, and no "
               "more than that fits through.")
         .emit();

        for (size_t k = 1; k < path.size(); k++) {
            int u = path[k - 1];
            int v = path[k];
            residual[u][v] -= delta;
            residual[v][u] += delta;

            bool backward = capacity[u][v] == 0;
            T.at(__LINE__, __func__)
             .vars({{"u", names[u]}, {"v", names[v]}, {"delta", delta}})
             .graphState(flowGraph(path, -1))
             .graphState("residual", residualGraph(path))
             .matrixState(residualMatrix(u, v))
             .relax(names[u], names[v], delta)
             .counters({{"paths", augmentations}, {"flow", maxFlow + delta}})
             .note(backward
                     ? std::string("BACKWARD edge ") + names[u] + "->" + names[v] + ": sending Δ here "
                       "means REMOVING " + std::to_string(delta) + " units from the flow already pushed on " +
                       names[v] + "->" + names[u] + ". This is how the algorithm undoes a wrong choice."
                     : std::string("Edge ") + names[u] + "->" + names[v] + ": the residual capacity drops by " +
                       std::to_string(delta) + " and in exchange the residual edge " + names[v] +
                       "->" + names[u] + " is born (or grows), which will allow undoing this choice if needed.")
             .emit();
        }

        maxFlow += delta;
        T.at(__LINE__, __func__)
         .vars({{"flow", maxFlow}})
         .graphState(flowGraph({}, -1))
         .graphState("residual", residualGraph({}))
         .matrixState(residualMatrix(-1, -1))
         .counters({{"paths", augmentations}, {"flow", maxFlow}})
         .note("Total flow: " + std::to_string(maxFlow) + ". Starting over to look for another path.")
         .emit();
    }
    return maxFlow;
}

static void reset() {
    for (int u = 0; u < N; u++)
        for (int v = 0; v < N; v++) capacity[u][v] = 0;
    maxFlow = 0;
    augmentations = 0;
}

int main() {
    T.view("graph").complexity("O(E · max flow)", "O(V²)");
    T.panel("graph", "Residual graph", "residual");
    T.panel("matrix", "Residual capacity");

    // Classic network: S -> {A, C}, T <- {B, D}.
    reset();
    capacity[0][1] = 10;   // S->A
    capacity[0][3] = 10;   // S->C
    capacity[1][2] = 4;    // A->B
    capacity[1][3] = 2;    // A->C
    capacity[1][4] = 8;    // A->D
    capacity[2][5] = 10;   // B->T
    capacity[3][4] = 9;    // C->D
    capacity[4][2] = 6;    // D->B
    capacity[4][5] = 10;   // D->T
    T.input("classic network");
    T.layout(layoutJson());
    fordFulkerson(0, 5);

    // Network built so the DFS immediately picks a path that later has to be
    // undone: here the backward edge is essential.
    reset();
    capacity[0][1] = 3;
    capacity[0][3] = 3;
    capacity[1][3] = 2;
    capacity[1][5] = 2;
    capacity[3][4] = 2;
    capacity[4][5] = 3;
    T.input("network where the backward edge is needed");
    T.layout(layoutJson());
    fordFulkerson(0, 5);

    T.dump("traces/ford_fulkerson.js");
    return 0;
}
