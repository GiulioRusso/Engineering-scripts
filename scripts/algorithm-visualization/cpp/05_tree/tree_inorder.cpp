// In-order traversal (DFS)
//
// Sinistra → radice → destra. Su un albero binario di ricerca questo ordine
// produce i valori in ordine crescente, ed è il modo più economico per
// verificare che un BST sia davvero un BST.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("tree_inorder", "In-order (DFS)", "Tree", __FILE__);

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

static Node* gRoot = nullptr;   // radice corrente: serve per disegnare sempre l'albero intero
static std::vector<int> output;
static std::vector<Node*> visited;

static std::string outputText() {
    std::string s;
    for (size_t i = 0; i < output.size(); ++i) s += (i ? " " : "") + std::to_string(output[i]);
    return "output: " + (s.empty() ? "(vuoto)" : s);
}

static trace::TreeMarks marks(Node* current) {
    trace::TreeMarks m;
    for (Node* v : visited) m.set(v, "visited");
    m.set(current, "current");
    return m;
}

void inorder(Node* node) {
    if (node == nullptr) {
        T.at(__LINE__, __func__)
         .treeState(trace::binaryTreeJson(gRoot, marks(nullptr)))
         .sequence(outputText())
         .note("Figlio nullo: la chiamata torna subito. È il caso base che ferma la ricorsione.")
         .emit();
        return;
    }

    T.pushFrame("inorder(" + std::to_string(node->data) + ")");
    T.at(__LINE__, __func__)
     .vars({{"node->data", node->data}})
     .treeState(trace::binaryTreeJson(gRoot, marks(node)))
     .sequence(outputText())
     .note("Entro nel nodo " + std::to_string(node->data) + ". Prima di stamparlo devo aver finito "
           "tutto il suo sottoalbero sinistro.")
     .emit();

    inorder(node->left);

    output.push_back(node->data);
    visited.push_back(node);
    T.at(__LINE__, __func__)
     .vars({{"node->data", node->data}})
     .treeState(trace::binaryTreeJson(gRoot, marks(node)))
     .visit(node->data)
     .sequence(outputText())
     .counters({{"visitati", (int)output.size()}})
     .note("Sottoalbero sinistro esaurito: stampo " + std::to_string(node->data) + ".")
     .emit();

    inorder(node->right);
    T.popFrame();
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

static Node* buildSmall() {
    Node* root = createNode(5);
    root->left = createNode(3);
    root->right = createNode(8);
    root->left->left = createNode(2);
    root->right->left = createNode(7);
    return root;
}

static void freeTree(Node* n) {
    if (!n) return;
    freeTree(n->left);
    freeTree(n->right);
    delete n;
}

static void run(const char* label, Node* root, const char* intro) {
    output.clear();
    visited.clear();
    gRoot = root;
    T.input(label);
    T.clearFrames();
    T.at(__LINE__, "inorder")
     .treeState(trace::binaryTreeJson(root))
     .sequence(outputText())
     .counters({{"visitati", 0}})
     .note(intro)
     .emit();

    inorder(root);

    T.at(__LINE__, "inorder")
     .treeState(trace::binaryTreeJson(gRoot, marks(nullptr)))
     .sequence(outputText())
     .counters({{"visitati", (int)output.size()}})
     .note("Attraversamento completo. Su un BST la sequenza è crescente: è la proprietà che rende "
           "l'in-order il controllo naturale di correttezza per un albero di ricerca.")
     .emit();
    freeTree(root);
}

int main() {
    T.view("tree").complexity("O(n)", "O(h)");
    T.panel("callstack", "Stack delle chiamate");

    trace::resetNodeIds();
    run("albero del libro", buildBook(), "Albero binario di ricerca. In-order: sinistra, radice, destra.");
    trace::resetNodeIds();
    run("albero piccolo", buildSmall(), "Un albero più piccolo, per seguire la ricorsione senza perdersi.");

    T.dump("traces/tree_inorder.js");
    return 0;
}
