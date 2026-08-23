// Ford-Fulkerson
//
// Flusso massimo da una sorgente a un pozzo. L'idea è semplice: finché esiste
// nel grafo RESIDUO un cammino dalla sorgente al pozzo, ci si spinge dentro
// tutto il flusso che ci sta — il collo di bottiglia Δ, cioè il minimo delle
// capacità residue lungo il cammino — e si ripete.
//
// Il punto che tutti sbagliano è l'arco ALL'INDIETRO. Quando si manda flusso da
// u a v, nel grafo residuo si aggiunge capacità da v a u. Quell'arco indietro
// non è un artificio contabile: usarlo significa DIMINUIRE il flusso su u->v,
// cioè disfare una scelta precedente. Senza quella possibilità l'algoritmo
// resterebbe incastrato in una prima decisione sbagliata.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("ford_fulkerson", "Ford-Fulkerson", "Flusso e MST", __FILE__);

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

// Grafo del flusso: etichette flow/capacity, come nel libro.
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

// Grafo residuo: gli archi indietro compaiono solo qui, ed è il motivo per cui
// vale la pena guardarlo.
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

// DFS nel grafo residuo: qualunque cammino va bene, non serve il più corto.
bool findPath(int u, int sink, std::vector<bool>& visited, std::vector<int>& path) {
    visited[u] = true;
    path.push_back(u);
    T.at(__LINE__, __func__)
     .vars({{"u", names[u]}})
     .graphState(flowGraph(path, u))
     .graphState("residuo", residualGraph(path))
     .matrixState(residualMatrix(u, -1))
     .counters({{"cammini", augmentations}, {"flusso", maxFlow}})
     .note("DFS nel residuo, sono in " + std::string(names[u]) +
           ". Cerco un arco con capacità residua ancora disponibile.")
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
     .graphState("residuo", residualGraph(path))
     .matrixState(residualMatrix(u, -1))
     .counters({{"cammini", augmentations}, {"flusso", maxFlow}})
     .note("Da " + std::string(names[u]) + " non si va da nessuna parte: torno indietro.")
     .emit();
    return false;
}

int fordFulkerson(int source, int sink) {
    for (int u = 0; u < N; u++)
        for (int v = 0; v < N; v++) residual[u][v] = capacity[u][v];

    T.at(__LINE__, __func__)
     .vars({{"flusso", maxFlow}})
     .graphState(flowGraph({}, -1))
     .graphState("residuo", residualGraph({}))
     .matrixState(residualMatrix(-1, -1))
     .counters({{"cammini", 0}, {"flusso", 0}})
     .note("Flusso zero ovunque: il residuo coincide con la capacità. Le etichette sono flow/capacity.")
     .emit();

    while (true) {
        std::vector<bool> visited(N, false);
        std::vector<int> path;
        if (!findPath(source, sink, visited, path)) {
            T.at(__LINE__, __func__)
             .vars({{"flusso massimo", maxFlow}})
             .graphState(flowGraph({}, -1))
             .graphState("residuo", residualGraph({}))
             .matrixState(residualMatrix(-1, -1))
             .counters({{"cammini", augmentations}, {"flusso", maxFlow}})
             .note("Nessun cammino aumentante: il flusso massimo è " + std::to_string(maxFlow) +
                   ". I nodi ancora raggiungibili dalla sorgente nel residuo formano il taglio minimo, "
                   "e la sua capacità è esattamente questo numero.")
             .emit();
            break;
        }

        int delta = 1000000;
        for (size_t k = 1; k < path.size(); k++)
            delta = std::min(delta, residual[path[k - 1]][path[k]]);

        augmentations++;
        T.at(__LINE__, __func__)
         .vars({{"cammino", pathText(path)}, {"delta", delta}})
         .graphState(flowGraph(path, -1))
         .graphState("residuo", residualGraph(path))
         .matrixState(residualMatrix(-1, -1))
         .counters({{"cammini", augmentations}, {"flusso", maxFlow}})
         .note("Cammino aumentante " + pathText(path) + ". Il collo di bottiglia è Δ = " +
               std::to_string(delta) + ": è la capacità residua più piccola lungo il cammino, e più di "
               "così non ci passa.")
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
             .graphState("residuo", residualGraph(path))
             .matrixState(residualMatrix(u, v))
             .relax(names[u], names[v], delta)
             .counters({{"cammini", augmentations}, {"flusso", maxFlow + delta}})
             .note(backward
                     ? std::string("Arco ALL'INDIETRO ") + names[u] + "->" + names[v] + ": mandare Δ qui "
                       "significa TOGLIERE " + std::to_string(delta) + " unità al flusso già spinto su " +
                       names[v] + "->" + names[u] + ". È così che l'algoritmo disfa una scelta sbagliata."
                     : std::string("Arco ") + names[u] + "->" + names[v] + ": la capacità residua scende di " +
                       std::to_string(delta) + " e in cambio nasce (o cresce) l'arco residuo " + names[v] +
                       "->" + names[u] + ", che permetterà di disfare questa scelta se servirà.")
             .emit();
        }

        maxFlow += delta;
        T.at(__LINE__, __func__)
         .vars({{"flusso", maxFlow}})
         .graphState(flowGraph({}, -1))
         .graphState("residuo", residualGraph({}))
         .matrixState(residualMatrix(-1, -1))
         .counters({{"cammini", augmentations}, {"flusso", maxFlow}})
         .note("Flusso totale: " + std::to_string(maxFlow) + ". Riparto a cercare un altro cammino.")
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
    T.view("graph").complexity("O(E · flusso massimo)", "O(V²)");
    T.panel("graph", "Grafo residuo", "residuo");
    T.panel("matrix", "Capacità residua");

    // Rete classica: S -> {A, C}, T <- {B, D}.
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
    T.input("rete classica");
    T.layout(layoutJson());
    fordFulkerson(0, 5);

    // Rete costruita perché la DFS scelga subito un cammino che poi va disfatto:
    // qui l'arco all'indietro è indispensabile.
    reset();
    capacity[0][1] = 3;
    capacity[0][3] = 3;
    capacity[1][3] = 2;
    capacity[1][5] = 2;
    capacity[3][4] = 2;
    capacity[4][5] = 3;
    T.input("rete in cui serve l'arco all'indietro");
    T.layout(layoutJson());
    fordFulkerson(0, 5);

    T.dump("traces/ford_fulkerson.js");
    return 0;
}
