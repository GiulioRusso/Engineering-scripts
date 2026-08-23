// Floyd-Warshall
//
// Cammini minimi fra TUTTE le coppie, non da una sola sorgente. Il contenuto
// dell'algoritmo è la matrice che evolve, non il grafo: per questo la vista
// principale qui è la matrice e il grafo è un pannello laterale.
//
// L'idea sta tutta nel significato del ciclo esterno: dopo la passata k, la
// cella [i][j] contiene il cammino minimo da i a j che può usare come nodi
// intermedi solo i primi k+1 vertici. Le passate producono la sequenza
// A⁰, A¹, A², ... e l'ultima è la risposta.
//
//   dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j])
#include <climits>
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"
#include "../tracer/graphs.hpp"

static Tracer T("floyd_warshall", "Floyd-Warshall", "Cammini minimi", __FILE__);

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

// I vertici ammessi come intermedi dopo la passata k.
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
     .counters({{"aggiornamenti", updates}})
     .note("A⁰: la matrice iniziale. Diagonale a 0, il peso dell'arco dove l'arco esiste, infinito "
           "altrove. Nessun nodo intermedio è ancora ammesso.")
     .emit();

    for (int k = 0; k < g.size(); k++) {
        T.at(__LINE__, __func__)
         .vars({{"k", g.names[k]}})
         .matrixState(matrixSnapshot(k, k, -1, -1))
         .graphState(graphSnapshot(k, -1, -1))
         .counters({{"aggiornamenti", updates}})
         .note("Passata k = " + g.names[k] + ". Da qui in poi " + g.names[k] +
               " è ammesso come nodo intermedio. La riga e la colonna " + g.names[k] +
               " sono i due soli ingredienti di ogni confronto di questa passata.")
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
                     .counters({{"aggiornamenti", updates}})
                     .note("Passando per " + g.names[k] + ": " + cellText(dist[i][k]) + " + " +
                           cellText(dist[k][j]) + " = " + cellText(dist[i][j]) + ", meglio di " +
                           cellText(before) + ". Aggiorno [" + g.names[i] + "][" + g.names[j] + "].")
                     .emit();
                }
            }
        }

        T.at(__LINE__, __func__)
         .vars({{"k", g.names[k]}})
         .matrixState(matrixSnapshot(-1, -1, -1, -1))
         .graphState(graphSnapshot(-1, -1, -1))
         .counters({{"aggiornamenti", updates}})
         .note("A" + sup(k + 1) + ": ogni cella è il cammino minimo che può passare solo per {" +
               allowed(k) + "} come nodi intermedi.")
         .emit();
    }

    T.at(__LINE__, __func__)
     .matrixState(matrixSnapshot(-1, -1, -1, -1))
     .graphState(graphSnapshot(-1, -1, -1))
     .counters({{"aggiornamenti", updates}})
     .note("Matrice finale: la distanza minima fra ogni coppia di vertici, in O(V³) tempo e O(V²) "
           "spazio, con tre cicli annidati e nessuna struttura ausiliaria.")
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
    T.view("matrix").complexity("O(V³)", "O(V²)");
    T.panel("graph", "Grafo");

    run(trace::directedGraph());
    run(trace::negativeEdgeGraph());

    T.dump("traces/floyd_warshall.js");
    return 0;
}
