// Depth-First Search on a Graph
//
// Goes as deep as it can along one branch, then backs up. The structure that
// enforces this order is the stack: the last node pushed is the first one
// out, so the search always descends into the neighbour it just discovered.
//
// Neighbours are pushed in DECREASING index order. That looks odd, and it's
// exactly to compensate for LIFO: pushing F, E, D pops D first, so the
// exploration order comes out increasing, the way the recursive version would.
//
// Backtracking is the moment a popped node's neighbours are all already
// visited: the stack shrinks and the search resumes from a branch left
// behind.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"
#include "../tracer/graphs.hpp"

static Tracer T("graph_dfs", "Depth-First Search", "Graphs", __FILE__);

static trace::GraphData g;
static bool visited[8];
static int visitOrder[8];
static int visitCount = 0;

// The stack from the Stack and Queue chapter.
static const int SCAP = 12;
static int stackData[SCAP];
static int top = -1;

void push(int v) {
    top++;
    stackData[top] = v;
}

int pop() {
    int v = stackData[top];
    top--;
    return v;
}

// tracer-hide-begin
static bool inStack(int v) {
    for (int k = 0; k <= top; k++) if (stackData[k] == v) return true;
    return false;
}

static std::string graphSnapshot(int current, int probing) {
    trace::GraphJson gj;
    for (int i = 0; i < g.size(); i++) {
        const char* st = "";
        if (i == current) st = "current";
        else if (visitOrder[i] > 0) st = "visited";
        else if (inStack(i)) st = "frontier";
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

static std::string stackTable() {
    std::string s = "{";
    for (int i = 0; i < g.size(); i++) {
        if (i) s += ",";
        s += trace::Val::quote(g.names[i]) + ":" + trace::Val::quote(inStack(i) ? "on stack" : "");
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

void dfs(int start) {
    push(start);
    T.at(__LINE__, __func__)
     .vars({{"start", g.names[start]}, {"top", top}})
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("Visited", visitedTable())
     .tableRaw("Stack", stackTable())
     .push(start)
     .sequence(order())
     .counters({{"visited", 0}})
     .note("Pushing the source " + g.names[start] + ". The stack holds nodes discovered but not yet visited.")
     .emit();

    while (top >= 0) {
        int u = pop();
        T.at(__LINE__, __func__)
         .vars({{"u", g.names[u]}, {"top", top}})
         .graphState(graphSnapshot(u, -1))
         .tableRaw("Visited", visitedTable())
         .tableRaw("Stack", stackTable())
         .pop()
         .sequence(order())
         .counters({{"visited", visitCount}})
         .note("Popping from the top: " + g.names[u] + ". Being the last one pushed, it's the \"deepest\" "
               "among those discovered.")
         .emit();

        if (visited[u]) {
            T.at(__LINE__, __func__)
             .vars({{"u", g.names[u]}, {"top", top}})
             .graphState(graphSnapshot(u, -1))
             .tableRaw("Visited", visitedTable())
             .tableRaw("Stack", stackTable())
             .sequence(order())
             .counters({{"visited", visitCount}})
             .note(g.names[u] + " was already visited via another path: discarding it. A node can end "
                   "up on the stack more than once, and this check is what makes the duplicate harmless.")
             .emit();
            continue;
        }

        visited[u] = true;
        visitOrder[u] = ++visitCount;
        T.at(__LINE__, __func__)
         .vars({{"u", g.names[u]}, {"top", top}})
         .graphState(graphSnapshot(u, -1))
         .tableRaw("Visited", visitedTable())
         .tableRaw("Stack", stackTable())
         .visit(g.names[u])
         .sequence(order())
         .counters({{"visited", visitCount}})
         .note("Visiting " + g.names[u] + ", node #" + std::to_string(visitCount) + ".")
         .emit();

        int pushed = 0;
        for (int v = g.size() - 1; v >= 0; v--) {
            if (g.adj[u][v] == 0 || visited[v]) continue;
            push(v);
            pushed++;
            T.at(__LINE__, __func__)
             .vars({{"u", g.names[u]}, {"v", g.names[v]}, {"top", top}})
             .graphState(graphSnapshot(u, v))
             .tableRaw("Visited", visitedTable())
             .tableRaw("Stack", stackTable())
             .push(v)
             .sequence(order())
             .counters({{"visited", visitCount}})
             .note("Pushing " + g.names[v] + ". The loop runs from " + std::to_string(g.size() - 1) +
                   " down to 0 precisely because the stack is LIFO: the smallest index, pushed last, "
                   "will be the first one out.")
             .emit();
        }

        if (pushed == 0) {
            T.at(__LINE__, __func__)
             .vars({{"u", g.names[u]}, {"top", top}})
             .graphState(graphSnapshot(u, -1))
             .tableRaw("Visited", visitedTable())
             .tableRaw("Stack", stackTable())
             .sequence(order())
             .counters({{"visited", visitCount}})
             .note("Dead end: every neighbour of " + g.names[u] + " is already visited. Nothing "
                   "pushed, so the next round pops a node left behind: this is "
                   "BACKTRACKING.")
             .emit();
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"top", top}})
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("Visited", visitedTable())
     .tableRaw("Stack", stackTable())
     .sequence(order())
     .counters({{"visited", visitCount}})
     .note("Stack empty: the visit is done. Compare the order with BFS's on the same graph — same "
           "nodes, completely different sequence.")
     .emit();
}

static void run(const trace::GraphData& src, int start) {
    g = src;
    for (int i = 0; i < 8; i++) { visited[i] = false; visitOrder[i] = 0; }
    visitCount = 0;
    top = -1;
    for (int i = 0; i < SCAP; i++) stackData[i] = 0;

    T.input(g.label + ", source " + g.names[start]);
    T.layout(g.layoutJson());
    T.watch("stack", [] {
        std::string s = "[";
        for (int i = 0; i < SCAP; i++) {
            if (i) s += ",";
            s += std::to_string(stackData[i]);   // vertex index, same as in the code
        }
        return s + "]";
    });
    T.at(__LINE__, "dfs")
     .vars({{"top", top}})
     .graphState(graphSnapshot(-1, -1))
     .tableRaw("Visited", visitedTable())
     .tableRaw("Stack", stackTable())
     .sequence(order())
     .counters({{"visited", 0}})
     .note("Initial graph, empty stack (top = -1).")
     .emit();

    dfs(start);
}

int main() {
    T.view("graph").complexity("O(V + E) with a list, O(V^2) with a matrix", "O(V)");
    T.panel("stack", "Stack of discovered nodes", "stack");
    T.panel("table", "Visited and stack");

    run(trace::traversalGraph(), 0);
    run(trace::cycleGraph(), 0);
    run(trace::traversalGraph(), 5);

    T.dump("traces/graph_dfs.js");
    return 0;
}
