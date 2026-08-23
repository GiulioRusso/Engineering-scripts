// BST: Search
//
// Ad ogni nodo una sola domanda, e la risposta butta via metà del lavoro: se la
// chiave è minore si scende a sinistra e tutto il sottoalbero destro non verrà
// mai guardato. Il costo è l'altezza dell'albero, O(h), non il numero di nodi.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("bst_search", "Search", "Binary Search Tree", __FILE__);

struct Node {
    int data;
    Node* left;
    Node* right;
};

Node* createNode(int value) {
    Node* node = new Node;
    node->data = value;
    node->left = node->right = nullptr;
    return node;
}

static Node* gRoot = nullptr;
static std::vector<Node*> path;      // nodi già attraversati
static std::vector<Node*> pruned;    // radici dei sottoalberi scartati
static int comparisons = 0;

static trace::TreeMarks marks(Node* current) {
    trace::TreeMarks m;
    for (Node* p : pruned) m.subtree(p, "pruned");
    for (Node* p : path) m.set(p, "path");
    m.set(current, "current");
    return m;
}

Node* search(Node* root, int key) {
    Node* current = root;

    while (current != nullptr) {
        T.at(__LINE__, __func__)
         .vars({{"key", key}, {"current->data", current->data}})
         .treeState(trace::binaryTreeJson(gRoot, marks(current)))
         .compare(0, -1)
         .counters({{"confronti", ++comparisons}})
         .note("Sono nel nodo " + std::to_string(current->data) + ". Confronto con la chiave " +
               std::to_string(key) + ": una sola domanda, tre risposte possibili.")
         .emit();

        if (key == current->data) {
            T.at(__LINE__, __func__)
             .vars({{"key", key}})
             .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(nullptr)).set(current, "found")))
             .found(0)
             .counters({{"confronti", comparisons}})
             .note("Trovata: " + std::to_string(key) + " era a profondità " + std::to_string(path.size()) +
                   ", raggiunta con " + std::to_string(comparisons) + " confronti.")
             .emit();
            return current;
        }

        path.push_back(current);
        if (key < current->data) {
            if (current->right) pruned.push_back(current->right);
            current = current->left;
            T.at(__LINE__, __func__)
             .vars({{"key", key}})
             .treeState(trace::binaryTreeJson(gRoot, marks(current)))
             .counters({{"confronti", comparisons}})
             .note(std::to_string(key) + " è minore: scendo a sinistra. Tutto il sottoalbero destro, "
                   "in grigio, non verrà mai esaminato — ed è questo il risparmio.")
             .emit();
        } else {
            if (current->left) pruned.push_back(current->left);
            current = current->right;
            T.at(__LINE__, __func__)
             .vars({{"key", key}})
             .treeState(trace::binaryTreeJson(gRoot, marks(current)))
             .counters({{"confronti", comparisons}})
             .note(std::to_string(key) + " è maggiore: scendo a destra, e il sottoalbero sinistro esce "
                   "di scena.")
             .emit();
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"key", key}})
     .treeState(trace::binaryTreeJson(gRoot, marks(nullptr)))
     .counters({{"confronti", comparisons}})
     .note("current è NULL: sono caduto fuori dall'albero, quindi " + std::to_string(key) +
           " non c'è. Ritorno nullptr.")
     .emit();
    return nullptr;
}

static Node* buildBook() {
    Node* root = createNode(8);
    root->left = createNode(3);
    root->right = createNode(10);
    root->left->left = createNode(1);
    root->left->right = createNode(6);
    root->left->right->left = createNode(4);
    root->left->right->right = createNode(7);
    root->right->right = createNode(14);
    root->right->right->left = createNode(13);
    return root;
}

static void freeTree(Node* n) {
    if (!n) return;
    freeTree(n->left);
    freeTree(n->right);
    delete n;
}

static void run(const char* label, int key, const char* intro) {
    trace::resetNodeIds();
    Node* root = buildBook();
    gRoot = root;
    path.clear();
    pruned.clear();
    comparisons = 0;

    T.input(label);
    T.at(__LINE__, "search")
     .vars({{"key", key}})
     .treeState(trace::binaryTreeJson(root))
     .counters({{"confronti", 0}})
     .note(intro)
     .emit();

    search(root, key);
    freeTree(root);
}

int main() {
    T.view("tree").complexity("O(h), O(log n) se bilanciato", "O(1)");

    run("cerca 4 (esempio del libro)", 4,
        "Albero del libro, 9 nodi. Cerco 4: bastano pochi confronti, non nove.");
    run("cerca 13 (a destra)", 13, "Cerco 13: la discesa va tutta a destra.");
    run("cerca 8 (la radice)", 8, "Cerco la radice: un solo confronto, il caso migliore.");
    run("cerca 5 (assente)", 5, "Cerco 5, che non c'è: la ricerca cade su un puntatore nullo.");

    T.dump("traces/bst_search.js");
    return 0;
}
