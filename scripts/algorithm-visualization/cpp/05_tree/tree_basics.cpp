// Tree n-ario: addChild, display, deleteTree
//
// Un albero generico non ha il vincolo dei due figli: ogni nodo tiene un
// vettore di figli, e il numero di figli è il suo grado. La radice è l'unico
// nodo senza padre, le foglie sono i nodi senza figli, la profondità di un nodo
// è la distanza dalla radice e l'altezza dell'albero è la profondità massima.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("tree_basics", "Tree n-ario: addChild, display, deleteTree", "Tree", __FILE__);

struct Node {
    int data;
    std::vector<Node*> children;
};

Node* createNode(int value) {
    Node* node = new Node;
    node->data = value;
    return node;
}

void addChild(Node* parent, Node* child) {
    parent->children.push_back(child);
}

int height(Node* node) {
    if (node == nullptr) return -1;
    int best = -1;
    for (Node* child : node->children) {
        int h = height(child);
        if (h > best) best = h;
    }
    return best + 1;
}

static Node* gRoot = nullptr;
static std::vector<int> output;

static std::string outputText() {
    std::string s;
    for (size_t i = 0; i < output.size(); ++i) s += (i ? " " : "") + std::to_string(output[i]);
    return "output: " + (s.empty() ? "(vuoto)" : s);
}

static trace::TreeMarks labelled() {
    trace::TreeMarks m;
    m.tag(gRoot, "root");
    return m;
}

// display: pre-order su un albero n-ario. Il nodo si stampa appena lo si
// incontra, poi si scende in tutti i figli da sinistra a destra.
void display(Node* node, int depth) {
    if (node == nullptr) return;

    output.push_back(node->data);
    T.pushFrame("display(" + std::to_string(node->data) + ", depth=" + std::to_string(depth) + ")");
    T.at(__LINE__, __func__)
     .vars({{"node->data", node->data}, {"depth", depth}, {"grado", (int)node->children.size()}})
     .treeState(trace::naryTreeJson(gRoot, trace::TreeMarks(labelled()).set(node, "current")))
     .visit(node->data)
     .sequence(outputText())
     .counters({{"visitati", (int)output.size()}})
     .note("Nodo " + std::to_string(node->data) + ": profondità " + std::to_string(depth) + ", grado " +
           std::to_string(node->children.size()) +
           (node->children.empty() ? ". È una foglia." : ". Scendo nei figli, da sinistra a destra."))
     .emit();

    for (Node* child : node->children) {
        display(child, depth + 1);
    }
    T.popFrame();
}

// deleteTree: post-order. I figli vanno liberati PRIMA del padre, altrimenti
// dopo la delete del padre il vettore children non esiste più e i figli
// diventano irraggiungibili. Ogni puntatore viene azzerato appena il figlio è
// stato liberato: nell'animazione si vede il sottoalbero sparire.
void deleteTree(Node* node) {
    if (node == nullptr) return;

    T.pushFrame("deleteTree(" + std::to_string(node->data) + ")");
    T.at(__LINE__, __func__)
     .vars({{"node->data", node->data}, {"figli", (int)node->children.size()}})
     .treeState(trace::naryTreeJson(gRoot, trace::TreeMarks(labelled()).set(node, "current")))
     .note("Entro in " + std::to_string(node->data) +
           " ma non lo libero ancora: prima devono sparire tutti i suoi figli.")
     .emit();

    for (size_t i = 0; i < node->children.size(); i++) {
        deleteTree(node->children[i]);
        node->children[i] = nullptr;   // niente puntatori penzolanti dopo la delete
    }

    T.at(__LINE__, __func__)
     .vars({{"node->data", node->data}})
     .treeState(trace::naryTreeJson(gRoot, trace::TreeMarks(labelled()).set(node, "doomed")))
     .note("I figli di " + std::to_string(node->data) + " sono stati liberati: ora tocca a lui. "
           "Questo è il post-order, ed è l'unico ordine sicuro per distruggere un albero.")
     .emit();
    T.popFrame();
    delete node;
}

int main() {
    T.view("tree").complexity("O(n)", "O(h)");
    T.panel("callstack", "Stack delle chiamate");

    // ------------------------------------------------ costruzione con addChild
    trace::resetNodeIds();
    Node* root = createNode(1);
    gRoot = root;
    output.clear();

    T.input("costruzione con addChild");
    T.at(__LINE__, "addChild")
     .vars({{"altezza", height(root)}, {"nodi", 1}})
     .treeState(trace::naryTreeJson(gRoot, labelled()))
     .note("Un solo nodo: è radice e foglia insieme. Altezza 0, grado 0.")
     .emit();

    const int level1[] = {2, 3, 4};
    for (int value : level1) {
        addChild(root, createNode(value));
        T.at(__LINE__, "addChild")
         .vars({{"altezza", height(root)}, {"grado radice", (int)root->children.size()}})
         .treeState(trace::naryTreeJson(gRoot, trace::TreeMarks(labelled()).set(root->children.back(), "found")))
         .note("addChild aggiunge " + std::to_string(value) +
               " in fondo al vettore dei figli della radice: nessun limite a due, il grado sale a " +
               std::to_string(root->children.size()) + ".")
         .emit();
    }

    const int level2[] = {5, 6};
    for (int value : level2) {
        addChild(root->children[0], createNode(value));
        T.at(__LINE__, "addChild")
         .vars({{"altezza", height(root)}})
         .treeState(trace::naryTreeJson(gRoot, trace::TreeMarks(labelled()).set(root->children[0]->children.back(), "found")))
         .note("Aggiungo " + std::to_string(value) + " sotto il nodo 2: l'altezza dell'albero sale a " +
               std::to_string(height(root)) + ", perché è la profondità massima di una foglia.")
         .emit();
    }

    // ---------------------------------------------------------------- display
    T.input("display (pre-order n-ario)");
    T.clearFrames();
    output.clear();
    T.at(__LINE__, "display")
     .vars({{"altezza", height(root)}})
     .treeState(trace::naryTreeJson(gRoot, labelled()))
     .sequence(outputText())
     .counters({{"visitati", 0}})
     .note("L'albero costruito sopra. display lo attraversa in pre-order.")
     .emit();
    display(root, 0);

    // ------------------------------------------------------------- deleteTree
    T.input("deleteTree (post-order)");
    T.clearFrames();
    T.at(__LINE__, "deleteTree")
     .treeState(trace::naryTreeJson(gRoot, labelled()))
     .note("Distruzione dell'albero. Guarda l'ordine: nessun nodo sparisce prima dei suoi figli.")
     .emit();
    deleteTree(root);
    gRoot = nullptr;
    T.at(__LINE__, "deleteTree")
     .treeState(trace::naryTreeJson<Node>(nullptr))
     .note("Albero vuoto: tutta la memoria è stata liberata, dalle foglie verso la radice.")
     .emit();

    T.dump("traces/tree_basics.js");
    return 0;
}
