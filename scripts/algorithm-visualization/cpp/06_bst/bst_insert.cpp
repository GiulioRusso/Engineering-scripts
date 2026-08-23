// BST: Insertion
//
// L'inserimento è la stessa discesa della ricerca: si cerca la chiave, e dove
// la ricerca fallirebbe — su un puntatore nullo — si aggancia il nodo nuovo.
// Ne segue che un nuovo nodo è sempre una foglia, e che l'ordine di
// inserimento determina la forma dell'albero.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("bst_insert", "Insertion", "Binary Search Tree", __FILE__);

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
static std::vector<Node*> path;
static int comparisons = 0;

static trace::TreeMarks marks(Node* current) {
    trace::TreeMarks m;
    for (Node* p : path) m.set(p, "path");
    m.set(current, "current");
    return m;
}

Node* insert(Node* root, int value) {
    T.pushFrame("insert(" + std::to_string(value) + ")");
    if (root == nullptr) {
        Node* fresh = createNode(value);
        gRoot = gRoot ? gRoot : fresh;
        T.at(__LINE__, __func__)
         .vars({{"value", value}})
         .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(nullptr)).set(fresh, "found")))
         .counters({{"confronti", comparisons}})
         .note("Puntatore nullo: è lo slot libero dove la ricerca sarebbe fallita. Ci atterra il nodo "
               "nuovo, che nasce sempre come foglia.")
         .emit();
        T.popFrame();
        return fresh;
    }

    T.at(__LINE__, __func__)
     .vars({{"value", value}, {"root->data", root->data}})
     .treeState(trace::binaryTreeJson(gRoot, marks(root)))
     .compare(0, -1)
     .counters({{"confronti", ++comparisons}})
     .note("Confronto " + std::to_string(value) + " con " + std::to_string(root->data) +
           ": stessa decisione della ricerca.")
     .emit();

    path.push_back(root);
    if (value < root->data) {
        root->left = insert(root->left, value);
    } else if (value > root->data) {
        root->right = insert(root->right, value);
    } else {
        T.at(__LINE__, __func__)
         .vars({{"value", value}})
         .treeState(trace::binaryTreeJson(gRoot, marks(root)))
         .error("duplicate")
         .counters({{"confronti", comparisons}})
         .note("Valore già presente: un BST classico non ammette duplicati, l'inserimento non fa nulla.")
         .emit();
    }
    T.popFrame();
    return root;
}

static void freeTree(Node* n) {
    if (!n) return;
    freeTree(n->left);
    freeTree(n->right);
    delete n;
}

static void run(const char* label, const std::vector<int>& values, const char* intro) {
    trace::resetNodeIds();
    Node* root = nullptr;
    gRoot = nullptr;
    comparisons = 0;

    T.input(label);
    T.at(__LINE__, "insert")
     .treeState(trace::binaryTreeJson<Node>(nullptr))
     .counters({{"confronti", 0}})
     .note(intro)
     .emit();

    for (int v : values) {
        path.clear();
        T.clearFrames();
        T.at(__LINE__, "insert")
         .vars({{"value", v}})
         .treeState(trace::binaryTreeJson(gRoot))
         .counters({{"confronti", comparisons}})
         .note("Inserisco " + std::to_string(v) + ": parto dalla radice.")
         .emit();
        root = insert(root, v);
        gRoot = root;
    }

    path.clear();
    T.at(__LINE__, "insert")
     .treeState(trace::binaryTreeJson(gRoot))
     .counters({{"confronti", comparisons}})
     .note("Albero completo. La sua forma dipende interamente dall'ordine in cui sono arrivati i valori.")
     .emit();
    freeTree(root);
}

int main() {
    T.view("tree").complexity("O(h), O(log n) se bilanciato", "O(h)");
    T.panel("callstack", "Stack delle chiamate");

    run("albero del libro", {8, 3, 10, 1, 6, 14, 4, 7, 13},
        "Albero vuoto. Inserisco i valori del libro, uno alla volta.");
    run("inserimento di un duplicato", {8, 3, 10, 3},
        "Gli ultimi due inserimenti sono lo stesso valore: il secondo non deve cambiare niente.");
    run("valori già ordinati (albero degenere)", {1, 2, 3, 4, 5, 6},
        "Valori crescenti: ogni nodo finisce a destra del precedente e l'albero degenera in una lista. "
        "Altezza n invece di log n, e la ricerca torna O(n).");

    T.dump("traces/bst_insert.js");
    return 0;
}
