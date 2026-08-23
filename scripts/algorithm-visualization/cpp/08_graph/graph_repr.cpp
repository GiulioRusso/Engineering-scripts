// Rappresentazione dei grafi: matrice ↔ lista di adiacenza
//
// Le due rappresentazioni contengono la stessa informazione con costi diversi.
// La matrice risponde in O(1) alla domanda "c'è un arco fra i e j?" ma occupa
// sempre V² celle, anche per un grafo con pochissimi archi. La lista occupa
// O(V+E) ma per la stessa domanda deve scorrere una lista.
//
// Qui le due vengono riempite insieme, un arco alla volta. Passando il mouse su
// un arco del grafo la vista salta al passo corrispondente, e si vede quali
// celle e quali voci di lista lo rappresentano.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"
#include "../tracer/graphs.hpp"

static Tracer T("graph_repr", "Adiacenza: matrice e lista", "Grafi", __FILE__);

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
    T.view("graph").complexity("matrice O(V²) spazio, lista O(V+E)", "—");
    T.panel("matrix", "Matrice di adiacenza");
    T.panel("table", "Lista di adiacenza");

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
         .tableRaw("adiacenti", listSnapshot())
         .counters({{"archi", 0}})
         .note("Grafo non orientato di " + std::to_string(g.size()) +
               " nodi. Matrice tutta a zero e liste vuote: le riempio un arco alla volta.")
         .emit();

        int edges = 0;
        for (int i = 0; i < g.size(); i++) {
            for (int j = i + 1; j < g.size(); j++) {
                if (!g.adj[i][j]) continue;

                // Un arco non orientato accende DUE celle della matrice: la
                // simmetria rispetto alla diagonale non è un dettaglio grafico,
                // è esattamente ciò che "non orientato" significa.
                matrix[i][j] = 1;
                matrix[j][i] = 1;
                adjList[i] += (adjList[i].empty() ? "" : ", ") + g.names[j];
                adjList[j] += (adjList[j].empty() ? "" : ", ") + g.names[i];
                edges++;

                T.at(__LINE__, "main")
                 .vars({{"i", i}, {"j", j}})
                 .graphState(graphSnapshot(i, j))
                 .matrixState(matrixSnapshot(i, j, i, j))
                 .tableRaw("adiacenti", listSnapshot())
                 .link(g.names[i], g.names[j])
                 .counters({{"archi", edges}})
                 .note("Arco " + g.names[i] + "-" + g.names[j] + ": accendo le celle [" + g.names[i] +
                       "][" + g.names[j] + "] e [" + g.names[j] + "][" + g.names[i] +
                       "], e aggiungo ciascuno alla lista dell'altro.")
                 .emit();
            }
        }

        T.at(__LINE__, "main")
         .vars({{"V", g.size()}, {"E", edges}, {"celle matrice", g.size() * g.size()}})
         .graphState(graphSnapshot(-1, -1))
         .matrixState(matrixSnapshot(-1, -1, -1, -1))
         .tableRaw("adiacenti", listSnapshot())
         .counters({{"archi", edges}})
         .note("Fatto. La matrice è simmetrica e occupa " + std::to_string(g.size() * g.size()) +
               " celle per soli " + std::to_string(edges) +
               " archi: su un grafo sparso è quasi tutto spreco, ed è il motivo per cui si usa la lista.")
         .emit();
    }

    T.dump("traces/graph_repr.js");
    return 0;
}
