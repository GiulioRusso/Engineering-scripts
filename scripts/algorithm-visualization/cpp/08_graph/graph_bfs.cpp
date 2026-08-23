// Breadth-First Search su grafo
//
// Visita per livelli: prima tutti i nodi a distanza 1 dalla sorgente, poi
// quelli a distanza 2, e così via. La struttura che impone quest'ordine è la
// coda: un nodo esce nell'ordine in cui è entrato, quindi i nodi vicini,
// entrati prima, escono prima.
//
// visited[] va messo a true al momento dell'ACCODAMENTO, non della visita.
// Altrimenti uno stesso nodo, raggiungibile da due vicini diversi, finirebbe in
// coda due volte.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"
#include "../tracer/graphs.hpp"

static Tracer T("graph_bfs", "Breadth-First Search", "Grafi", __FILE__);

static trace::GraphData g;
static bool visited[8];
static int visitOrder[8];
static int visitCount = 0;

// La coda circolare del capitolo Stack e Queue.
static const int QCAP = 8;
static int queueData[QCAP];
static int front = 0, rear = -1, count = 0;

void enqueue(int v) {
    rear = (rear + 1) % QCAP;
    queueData[rear] = v;
    count++;
}

int dequeue() {
    int v = queueData[front];
    front = (front + 1) % QCAP;
    count--;
    return v;
}

// tracer-hide-begin
static bool inQueue(int v) {
    for (int k = 0; k < count; k++) if (queueData[(front + k) % QCAP] == v) return true;
    return false;
}

static std::string graphSnapshot(int current, int probing) {
    trace::GraphJson gj;
    for (int i = 0; i < g.size(); i++) {
        const char* st = "";
        if (i == current) st = "current";
        else if (visitOrder[i] > 0) st = "visited";
        else if (inQueue(i)) st = "frontier";
        gj.node(g.names[i], st);
    }
    for (int i = 0; i < g.size(); i++)
        for (int j = i + 1; j < g.size(); j++)
            if (g.adj[i][j]) {
                bool hot = (i == current && j == probing) || (j == current && i == probing);
                gj.edge(g.names[i], g.names[j], "", hot ? "current" : "");
            }
    return gj.str();
}

static std::string visitedTable() {
    std::string s = "{";
    for (int i = 0; i < g.size(); i++) {
        if (i) s += ",";
        s += trace::Val::quote(g.names[i]) + ":" +
             trace::Val::quote(visitOrder[i] > 0 ? std::to_string(visitOrder[i]) : "-");
    }
    return s + "}";
}

static std::string queueTable() {
    std::string s = "{";
    for (int i = 0; i < g.size(); i++) {
        if (i) s += ",";
        s += trace::Val::quote(g.names[i]) + ":" + trace::Val::quote(inQueue(i) ? "in coda" : "");
    }
    return s + "}";
}

static std::string order() {
    std::string s;
    for (int k = 1; k <= visitCount; k++)
        for (int i = 0; i < g.size(); i++)
            if (visitOrder[i] == k) s += (s.empty() ? "" : " ") + g.names[i];
    return "Visited: " + (s.empty() ? "(vuoto)" : s);
}
// tracer-hide-end

void bfs(int start) {
    visited[start] = true;
    enqueue(start);
    T.at(__LINE__, __func__)
     .vars({{"start", g.names[start]}, {"front", front}, {"rear", rear}, {"count", count}})
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("Visited", visitedTable())
     .tableRaw("Coda", queueTable())
     .enqueue(start)
     .sequence(order())
     .counters({{"visitati", visitCount}})
     .note("Accodo la sorgente " + g.names[start] +
           " e la marco subito come visitata: marcare all'accodamento evita che finisca in coda due "
           "volte se due vicini la raggiungono entrambi.")
     .emit();

    while (count > 0) {
        int u = dequeue();
        visitOrder[u] = ++visitCount;
        T.at(__LINE__, __func__)
         .vars({{"u", g.names[u]}, {"front", front}, {"rear", rear}, {"count", count}})
         .graphState(graphSnapshot(u, -1))
         .tableRaw("Visited", visitedTable())
         .tableRaw("Coda", queueTable())
         .visit(g.names[u])
         .sequence(order())
         .counters({{"visitati", visitCount}})
         .note("Estraggo dalla testa e visito " + g.names[u] + ". È il " + std::to_string(visitCount) +
               "° nodo visitato, e la riga Visited si riempie in quest'ordine.")
         .emit();

        for (int v = 0; v < g.size(); v++) {
            if (g.adj[u][v] == 0) continue;

            if (!visited[v]) {
                visited[v] = true;
                enqueue(v);
                T.at(__LINE__, __func__)
                 .vars({{"u", g.names[u]}, {"v", g.names[v]}, {"count", count}})
                 .graphState(graphSnapshot(u, v))
                 .tableRaw("Visited", visitedTable())
                 .tableRaw("Coda", queueTable())
                 .enqueue(v)
                 .sequence(order())
                 .counters({{"visitati", visitCount}})
                 .note(g.names[v] + " non era ancora visto: entra in frontiera, in fondo alla coda. "
                       "Uscirà dopo tutti quelli già presenti, cioè dopo il resto di questo livello.")
                 .emit();
            } else {
                T.at(__LINE__, __func__)
                 .vars({{"u", g.names[u]}, {"v", g.names[v]}})
                 .graphState(graphSnapshot(u, v))
                 .tableRaw("Visited", visitedTable())
                 .tableRaw("Coda", queueTable())
                 .sequence(order())
                 .counters({{"visitati", visitCount}})
                 .note(g.names[v] + " è già visitato o già in coda: lo salto. Senza questo controllo un "
                       "ciclo farebbe girare la BFS all'infinito.")
                 .emit();
            }
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"count", count}})
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("Visited", visitedTable())
     .tableRaw("Coda", queueTable())
     .sequence(order())
     .counters({{"visitati", visitCount}})
     .note("Coda vuota: tutti i nodi raggiungibili dalla sorgente sono stati visitati, in ordine di "
           "distanza crescente.")
     .emit();
}

static void run(const trace::GraphData& src, int start) {
    g = src;
    for (int i = 0; i < 8; i++) { visited[i] = false; visitOrder[i] = 0; }
    visitCount = 0;
    front = 0;
    rear = -1;
    count = 0;
    for (int i = 0; i < QCAP; i++) queueData[i] = 0;

    T.input(g.label + ", sorgente " + g.names[start]);
    T.layout(g.layoutJson());
    T.watch("queue", [] {
        std::string s = "[";
        for (int i = 0; i < QCAP; i++) {
            if (i) s += ",";
            s += std::to_string(queueData[i]);   // indice del vertice, come nel codice
        }
        return s + "]";
    });
    T.at(__LINE__, "bfs")
     .vars({{"front", front}, {"rear", rear}, {"count", count}})
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("Visited", visitedTable())
     .tableRaw("Coda", queueTable())
     .sequence(order())
     .counters({{"visitati", 0}})
     .note("Grafo iniziale, nessun nodo visitato, coda vuota.")
     .emit();

    bfs(start);
}

int main() {
    T.view("graph").complexity("O(V + E) con lista, O(V²) con matrice", "O(V)");
    T.panel("queue", "Coda (frontiera)", "queue");
    T.panel("table", "Visited e coda");

    run(trace::traversalGraph(), 0);
    run(trace::cycleGraph(), 0);
    run(trace::traversalGraph(), 5);

    T.dump("traces/graph_bfs.js");
    return 0;
}
