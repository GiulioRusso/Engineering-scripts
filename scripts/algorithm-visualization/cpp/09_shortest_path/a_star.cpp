// A*
//
// Dijkstra che sa dove sta andando. Invece di espandere sempre il nodo più
// vicino alla sorgente, espande quello che minimizza
//
//   f(n) = g(n) + h(n)
//
// dove g è il costo REALE già percorso e h è una STIMA del costo che resta fino
// alla meta. Se h non sovrastima mai il costo vero — cioè è ammissibile — il
// cammino trovato è ottimo esattamente come in Dijkstra, ma vengono espansi
// molti meno nodi, perché la stima scoraggia le direzioni sbagliate.
//
// Con h ≡ 0, A* è Dijkstra. Più h è informata, meno nodi si aprono.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("a_star", "A*", "Cammini minimi", __FILE__);

static const int INF = 1000000;
static const int N = 6;

// Nodi su una griglia: i pesi degli archi sono le distanze reali fra le celle,
// così l'euristica di Manhattan non può sovrastimare ed è ammissibile per
// costruzione.
static const char* names[N] = {"S", "A", "B", "C", "D", "G"};
static const int gx[N] = {0, 0, 2, 2, 4, 6};
static const int gy[N] = {0, 2, 0, 2, 1, 1};
static int adj[N][N];

enum Heuristic { MANHATTAN, EUCLIDEA, ZERO };
static Heuristic mode = MANHATTAN;

int heuristic(int n, int goal) {
    if (mode == ZERO) return 0;   // A* con h = 0 è esattamente Dijkstra
    int dx = std::abs(gx[n] - gx[goal]);
    int dy = std::abs(gy[n] - gy[goal]);
    if (mode == MANHATTAN) return dx + dy;
    // Arrotondata per difetto: una stima che non sovrastima mai resta ammissibile.
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
         .counters({{"espansioni", expansions}})
         .note(std::string("Euristica ") +
               (mode == MANHATTAN ? "di Manhattan" : mode == EUCLIDEA ? "euclidea" : "nulla (h = 0)") +
               ". Solo la sorgente è nell'open set: g=0, f = 0 + h = " + std::to_string(fScore[start]) +
               ". Le h sono già calcolate per tutti i nodi e non cambiano mai.");
        emitTables(b);
        b.emit();
    }

    while (true) {
        int u = lowestF();
        if (u < 0) {
            auto b = T.at(__LINE__, __func__);
            b.graphState(graphSnapshot(-1, -1, {}))
             .error("nessun_cammino")
             .counters({{"espansioni", expansions}})
             .note("Open set vuoto senza aver raggiunto la meta: non esiste un cammino.");
            emitTables(b);
            b.emit();
            return;
        }

        {
            auto b = T.at(__LINE__, __func__);
            b.vars({{"u", names[u]}, {"g", num(gScore[u])}, {"h", hScore[u]}, {"f", num(fScore[u])}})
             .graphState(graphSnapshot(u, -1, {}))
             .counters({{"espansioni", expansions}})
             .note("Espando il nodo con f minima: " + std::string(names[u]) + ", f = " +
                   num(gScore[u]) + " + " + std::to_string(hScore[u]) + " = " + num(fScore[u]) +
                   ". Dijkstra qui userebbe solo g e prenderebbe una strada diversa.");
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
                 .counters({{"espansioni", expansions}})
                 .note(std::string(names[v]) + " entra (o migliora) nell'open set: g = " +
                       std::to_string(tentative) + ", h = " + std::to_string(hScore[v]) + ", quindi f = " +
                       std::to_string(fScore[v]) + ".");
                emitTables(b);
                b.emit();
            } else {
                auto b = T.at(__LINE__, __func__);
                b.vars({{"u", names[u]}, {"v", names[v]}, {"tentative", tentative}})
                 .graphState(graphSnapshot(u, v, {}))
                 .counters({{"espansioni", expansions}})
                 .note("Arrivare a " + std::string(names[v]) + " passando per " + names[u] +
                       " costerebbe " + std::to_string(tentative) + ", non meno di " + num(gScore[v]) +
                       ": lascio stare.");
                emitTables(b);
                b.emit();
            }
        }
    }

    // Ricostruzione a ritroso seguendo parent[].
    std::vector<int> path;
    for (int at = goal; at != -1; at = parent[at]) path.insert(path.begin(), at);

    std::string text;
    for (size_t i = 0; i < path.size(); i++) text += (i ? " -> " : "") + std::string(names[path[i]]);
    {
        auto b = T.at(__LINE__, __func__);
        b.vars({{"costo", num(gScore[goal])}, {"espansioni", expansions}})
         .graphState(graphSnapshot(-1, -1, path))
         .sequence("cammino: " + text)
         .counters({{"espansioni", expansions}})
         .note("Meta raggiunta. Il cammino si ricostruisce a ritroso da parent[]: " + text +
               ", costo " + num(gScore[goal]) + ", con " + std::to_string(expansions) +
               " nodi espansi.");
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
    T.view("graph").complexity("O(E) con euristica perfetta, O(V²) nel peggiore", "O(V)");
    T.panel("table", "gScore, hScore, fScore e parent");

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) adj[i][j] = 0;
    // Peso di ogni arco = distanza di Manhattan fra le due celle della griglia.
    const int links[][2] = {{0, 1}, {0, 2}, {1, 3}, {2, 3}, {2, 4}, {3, 4}, {4, 5}};
    for (const auto& l : links) {
        int w = std::abs(gx[l[0]] - gx[l[1]]) + std::abs(gy[l[0]] - gy[l[1]]);
        adj[l[0]][l[1]] = w;
        adj[l[1]][l[0]] = w;
    }

    run("euristica di Manhattan", MANHATTAN);
    run("euristica euclidea", EUCLIDEA);
    run("h = 0 (cioè Dijkstra)", ZERO);

    T.dump("traces/a_star.js");
    return 0;
}
