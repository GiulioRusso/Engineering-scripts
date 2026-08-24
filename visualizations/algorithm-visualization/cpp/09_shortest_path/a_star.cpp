// A*
//
// Dijkstra that knows where it's going. Instead of always expanding the node
// closest to the source, it expands whichever minimizes
//
//   f(n) = g(n) + h(n)
//
// where g is the REAL cost travelled so far and h is an ESTIMATE of the cost
// remaining to the goal. If h never overestimates the true cost — that is, it
// is admissible — the path found is just as optimal as Dijkstra's, but far
// fewer nodes get expanded, because the estimate discourages the wrong
// directions.
//
// With h == 0, A* is Dijkstra. The more informed h is, the fewer nodes open.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("a_star", "A*", "Shortest Paths", __FILE__);

static const int INF = 1000000;
static const int N = 6;

// Nodes on a grid: edge weights are the real distances between cells, so the
// Manhattan heuristic can never overestimate and is admissible by
// construction.
static const char* names[N] = {"S", "A", "B", "C", "D", "G"};
static const int gx[N] = {0, 0, 2, 2, 4, 6};
static const int gy[N] = {0, 2, 0, 2, 1, 1};
static int adj[N][N];

enum Heuristic { MANHATTAN, EUCLIDEA, ZERO };
static Heuristic mode = MANHATTAN;

int heuristic(int n, int goal) {
    if (mode == ZERO) return 0;   // A* with h = 0 is exactly Dijkstra
    int dx = std::abs(gx[n] - gx[goal]);
    int dy = std::abs(gy[n] - gy[goal]);
    if (mode == MANHATTAN) return dx + dy;
    // Rounded down: an estimate that never overestimates stays admissible.
    return static_cast<int>(std::sqrt(dx * dx + dy * dy));
}

static int gScore[N], fScore[N], hScore[N], parent[N];
static bool inOpen[N], inClosed[N];
static int expansions = 0;

// tracer-hide-begin
static std::string num(int v) { return v >= INF ? "inf" : std::to_string(v); }

static std::string layoutJson() {
    std::string s = "{";
    for (int i = 0; i < N; i++) {
        if (i) s += ",";
        s += trace::Val::quote(names[i]) + ":{\"x\":" + std::to_string(gx[i] * 72) +
             ",\"y\":" + std::to_string(gy[i] * 72) + "}";
    }
    return s + "}";
}

static std::string tableOf(const int* arr) {
    std::string s = "{";
    for (int i = 0; i < N; i++) {
        if (i) s += ",";
        s += trace::Val::quote(names[i]) + ":" + trace::Val::quote(num(arr[i]));
    }
    return s + "}";
}

static std::string parentTable() {
    std::string s = "{";
    for (int i = 0; i < N; i++) {
        if (i) s += ",";
        s += trace::Val::quote(names[i]) + ":" +
             trace::Val::quote(parent[i] < 0 ? "-" : names[parent[i]]);
    }
    return s + "}";
}

static std::string graphSnapshot(int current, int probing, const std::vector<int>& path) {
    trace::GraphJson gj;
    for (int i = 0; i < N; i++) {
        const char* st = "";
        bool onPath = false;
        for (int p : path) if (p == i) onPath = true;
        if (onPath) st = "path";
        else if (i == current) st = "current";
        else if (inClosed[i]) st = "visited";
        else if (inOpen[i]) st = "frontier";
        std::string label;
        if (gScore[i] < INF)
            label = "f=" + num(gScore[i]) + "+" + std::to_string(hScore[i]) + "=" + num(fScore[i]);
        gj.node(names[i], st, label);
    }
    for (int i = 0; i < N; i++)
        for (int j = i + 1; j < N; j++)
            if (adj[i][j]) {
                bool hot = (i == current && j == probing) || (j == current && i == probing);
                bool onPath = false;
                for (size_t k = 1; k < path.size(); k++)
                    if ((path[k - 1] == i && path[k] == j) || (path[k - 1] == j && path[k] == i))
                        onPath = true;
                gj.edge(names[i], names[j], std::to_string(adj[i][j]),
                        onPath ? "accepted" : (hot ? "current" : ""));
            }
    return gj.str();
}

static void emitTables(trace::StepBuilder& b) {
    b.tableRaw("gScore", tableOf(gScore))
     .tableRaw("hScore", tableOf(hScore))
     .tableRaw("fScore", tableOf(fScore))
     .tableRaw("parent", parentTable());
}
// tracer-hide-end

int lowestF() {
    int best = INF, index = -1;
    for (int i = 0; i < N; i++) {
        if (inOpen[i] && fScore[i] < best) {
            best = fScore[i];
            index = i;
        }
    }
    return index;
}

void aStar(int start, int goal) {
    for (int i = 0; i < N; i++) {
        gScore[i] = INF;
        fScore[i] = INF;
        hScore[i] = heuristic(i, goal);
        parent[i] = -1;
        inOpen[i] = false;
        inClosed[i] = false;
    }
    gScore[start] = 0;
    fScore[start] = hScore[start];
    inOpen[start] = true;
    {
        auto b = T.at(__LINE__, __func__);
        b.vars({{"start", names[start]}, {"goal", names[goal]}, {"h(start)", hScore[start]}})
         .graphState(graphSnapshot(-1, -1, {}))
         .counters({{"expansions", expansions}})
         .note(std::string("Heuristic: ") +
               (mode == MANHATTAN ? "Manhattan" : mode == EUCLIDEA ? "Euclidean" : "none (h = 0)") +
               ". Only the source is in the open set: g=0, f = 0 + h = " + std::to_string(fScore[start]) +
               ". h is already computed for every node and never changes.");
        emitTables(b);
        b.emit();
    }

    while (true) {
        int u = lowestF();
        if (u < 0) {
            auto b = T.at(__LINE__, __func__);
            b.graphState(graphSnapshot(-1, -1, {}))
             .error("no_path")
             .counters({{"expansions", expansions}})
             .note("Open set empty without having reached the goal: no path exists.");
            emitTables(b);
            b.emit();
            return;
        }

        {
            auto b = T.at(__LINE__, __func__);
            b.vars({{"u", names[u]}, {"g", num(gScore[u])}, {"h", hScore[u]}, {"f", num(fScore[u])}})
             .graphState(graphSnapshot(u, -1, {}))
             .counters({{"expansions", expansions}})
             .note("Expanding the node with lowest f: " + std::string(names[u]) + ", f = " +
                   num(gScore[u]) + " + " + std::to_string(hScore[u]) + " = " + num(fScore[u]) +
                   ". Dijkstra would use only g here and take a different route.");
            emitTables(b);
            b.emit();
        }

        if (u == goal) break;

        inOpen[u] = false;
        inClosed[u] = true;
        expansions++;

        for (int v = 0; v < N; v++) {
            if (adj[u][v] == 0 || inClosed[v]) continue;

            int tentative = gScore[u] + adj[u][v];
            if (tentative < gScore[v]) {
                parent[v] = u;
                gScore[v] = tentative;
                fScore[v] = tentative + hScore[v];
                inOpen[v] = true;
                auto b = T.at(__LINE__, __func__);
                b.vars({{"u", names[u]}, {"v", names[v]}, {"g", tentative}, {"h", hScore[v]},
                        {"f", fScore[v]}})
                 .graphState(graphSnapshot(u, v, {}))
                 .relax(names[u], names[v], fScore[v])
                 .counters({{"expansions", expansions}})
                 .note(std::string(names[v]) + " enters (or improves) the open set: g = " +
                       std::to_string(tentative) + ", h = " + std::to_string(hScore[v]) + ", so f = " +
                       std::to_string(fScore[v]) + ".");
                emitTables(b);
                b.emit();
            } else {
                auto b = T.at(__LINE__, __func__);
                b.vars({{"u", names[u]}, {"v", names[v]}, {"tentative", tentative}})
                 .graphState(graphSnapshot(u, v, {}))
                 .counters({{"expansions", expansions}})
                 .note("Reaching " + std::string(names[v]) + " through " + names[u] +
                       " would cost " + std::to_string(tentative) + ", not less than " + num(gScore[v]) +
                       ": leaving it alone.");
                emitTables(b);
                b.emit();
            }
        }
    }

    // Reconstruct backwards by following parent[].
    std::vector<int> path;
    for (int at = goal; at != -1; at = parent[at]) path.insert(path.begin(), at);

    std::string text;
    for (size_t i = 0; i < path.size(); i++) text += (i ? " -> " : "") + std::string(names[path[i]]);
    {
        auto b = T.at(__LINE__, __func__);
        b.vars({{"cost", num(gScore[goal])}, {"expansions", expansions}})
         .graphState(graphSnapshot(-1, -1, path))
         .sequence("path: " + text)
         .counters({{"expansions", expansions}})
         .note("Goal reached. The path is reconstructed backwards from parent[]: " + text +
               ", cost " + num(gScore[goal]) + ", with " + std::to_string(expansions) +
               " nodes expanded.");
        emitTables(b);
        b.emit();
    }
}

static void run(const char* label, Heuristic h) {
    mode = h;
    expansions = 0;
    T.input(label);
    T.layout(layoutJson());
    aStar(0, 5);
}

int main() {
    T.view("graph").complexity("O(E) with a perfect heuristic, O(V^2) worst case", "O(V)");
    T.panel("table", "gScore, hScore, fScore and parent");

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) adj[i][j] = 0;
    // Each edge's weight = the Manhattan distance between the two grid cells.
    const int links[][2] = {{0, 1}, {0, 2}, {1, 3}, {2, 3}, {2, 4}, {3, 4}, {4, 5}};
    for (const auto& l : links) {
        int w = std::abs(gx[l[0]] - gx[l[1]]) + std::abs(gy[l[0]] - gy[l[1]]);
        adj[l[0]][l[1]] = w;
        adj[l[1]][l[0]] = w;
    }

    run("Manhattan heuristic", MANHATTAN);
    run("Euclidean heuristic", EUCLIDEA);
    run("h = 0 (i.e. Dijkstra)", ZERO);

    T.dump("traces/a_star.js");
    return 0;
}
