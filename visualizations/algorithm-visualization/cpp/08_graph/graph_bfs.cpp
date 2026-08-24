// Breadth-First Search on a Graph
//
// Level-by-level visit: first every node at distance 1 from the source, then
// every node at distance 2, and so on. The structure that enforces this order
// is the queue: a node comes out in the order it went in, so nearer nodes,
// enqueued earlier, come out earlier.
//
// visited[] has to be set true at ENQUEUE time, not at visit time. Otherwise
// the same node, reachable from two different neighbours, would end up in the
// queue twice.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"
#include "../tracer/graphs.hpp"

static Tracer T("graph_bfs", "Breadth-First Search", "Graphs", __FILE__);

static trace::GraphData g;
static bool visited[8];
static int visitOrder[8];
static int visitCount = 0;

// The circular queue from the Stack and Queue chapter.
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
        s += trace::Val::quote(g.names[i]) + ":" + trace::Val::quote(inQueue(i) ? "in queue" : "");
    }
    return s + "}";
}

static std::string order() {
    std::string s;
    for (int k = 1; k <= visitCount; k++)
        for (int i = 0; i < g.size(); i++)
            if (visitOrder[i] == k) s += (s.empty() ? "" : " ") + g.names[i];
    return "Visited: " + (s.empty() ? "(empty)" : s);
}
// tracer-hide-end

void bfs(int start) {
    visited[start] = true;
    enqueue(start);
    T.at(__LINE__, __func__)
     .vars({{"start", g.names[start]}, {"front", front}, {"rear", rear}, {"count", count}})
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("Visited", visitedTable())
     .tableRaw("Queue", queueTable())
     .enqueue(start)
     .sequence(order())
     .counters({{"visited", visitCount}})
     .note("Enqueueing the source " + g.names[start] +
           " and marking it visited right away: marking at enqueue time avoids it ending up in the "
           "queue twice if two neighbours both reach it.")
     .emit();

    while (count > 0) {
        int u = dequeue();
        visitOrder[u] = ++visitCount;
        T.at(__LINE__, __func__)
         .vars({{"u", g.names[u]}, {"front", front}, {"rear", rear}, {"count", count}})
         .graphState(graphSnapshot(u, -1))
         .tableRaw("Visited", visitedTable())
         .tableRaw("Queue", queueTable())
         .visit(g.names[u])
         .sequence(order())
         .counters({{"visited", visitCount}})
         .note("Dequeuing from the front and visiting " + g.names[u] + ". This is the " +
               std::to_string(visitCount) + "th node visited, and the Visited row fills in this order.")
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
                 .tableRaw("Queue", queueTable())
                 .enqueue(v)
                 .sequence(order())
                 .counters({{"visited", visitCount}})
                 .note(g.names[v] + " hadn't been seen yet: it enters the frontier, at the back of the "
                       "queue. It will come out after everyone already there, i.e. after the rest of this level.")
                 .emit();
            } else {
                T.at(__LINE__, __func__)
                 .vars({{"u", g.names[u]}, {"v", g.names[v]}})
                 .graphState(graphSnapshot(u, v))
                 .tableRaw("Visited", visitedTable())
                 .tableRaw("Queue", queueTable())
                 .sequence(order())
                 .counters({{"visited", visitCount}})
                 .note(g.names[v] + " is already visited or already in the queue: skipping it. Without "
                       "this check a cycle would send BFS spinning forever.")
                 .emit();
            }
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"count", count}})
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("Visited", visitedTable())
     .tableRaw("Queue", queueTable())
     .sequence(order())
     .counters({{"visited", visitCount}})
     .note("Queue empty: every node reachable from the source has been visited, in order of "
           "increasing distance.")
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

    T.input(g.label + ", source " + g.names[start]);
    T.layout(g.layoutJson());
    T.watch("queue", [] {
        std::string s = "[";
        for (int i = 0; i < QCAP; i++) {
            if (i) s += ",";
            s += std::to_string(queueData[i]);   // vertex index, same as in the code
        }
        return s + "]";
    });
    T.at(__LINE__, "bfs")
     .vars({{"front", front}, {"rear", rear}, {"count", count}})
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("Visited", visitedTable())
     .tableRaw("Queue", queueTable())
     .sequence(order())
     .counters({{"visited", 0}})
     .note("Initial graph, no node visited, empty queue.")
     .emit();

    bfs(start);
}

int main() {
    T.view("graph").complexity("O(V + E) with a list, O(V^2) with a matrix", "O(V)");
    T.panel("queue", "Queue (frontier)", "queue");
    T.panel("table", "Visited and queue");

    run(trace::traversalGraph(), 0);
    run(trace::cycleGraph(), 0);
    run(trace::traversalGraph(), 5);

    T.dump("traces/graph_bfs.js");
    return 0;
}
