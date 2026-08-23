// BST: Delete (i tre casi)
//
// È il pezzo più delicato del capitolo, perché la cancellazione deve lasciare
// intatta la proprietà del BST. Tre casi, e la traccia dichiara sempre quale
// si sta applicando:
//
//   1. foglia          → si stacca e basta
//   2. un solo figlio  → il figlio prende il posto del padre
//   3. due figli       → si copia il successore in-order (il minimo del
//                        sottoalbero destro) e si cancella quello, che per
//                        costruzione ricade nel caso 1 o nel caso 2
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("bst_delete", "Delete (3 casi)", "Binary Search Tree", __FILE__);

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

static trace::TreeMarks marks(Node* current) {
    trace::TreeMarks m;
    for (Node* p : path) m.set(p, "path");
    m.set(current, "current");
    return m;
}

// Il minimo di un sottoalbero è il nodo più a sinistra: non ha figlio sinistro,
// altrimenti non sarebbe il minimo.
Node* findMin(Node* node) {
    while (node->left != nullptr) {
        node = node->left;
        T.at(__LINE__, __func__)
         .vars({{"node->data", node->data}})
         .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(node)).set(node, "moved")))
         .note("Scendo ancora a sinistra: il successore in-order è il nodo più a sinistra del "
               "sottoalbero destro.")
         .emit();
    }
    return node;
}

Node* deleteNode(Node* root, int key) {
    if (root == nullptr) {
        T.at(__LINE__, __func__)
         .vars({{"key", key}})
         .treeState(trace::binaryTreeJson(gRoot, marks(nullptr)))
         .error("not_found")
         .note("Puntatore nullo: la chiave " + std::to_string(key) + " non è nell'albero.")
         .emit();
        return nullptr;
    }

    T.at(__LINE__, __func__)
     .vars({{"key", key}, {"root->data", root->data}})
     .treeState(trace::binaryTreeJson(gRoot, marks(root)))
     .compare(0, -1)
     .note("Cerco " + std::to_string(key) + ", sono nel nodo " + std::to_string(root->data) + ".")
     .emit();

    if (key < root->data) {
        path.push_back(root);
        root->left = deleteNode(root->left, key);
    } else if (key > root->data) {
        path.push_back(root);
        root->right = deleteNode(root->right, key);
    } else {
        // ---------------------------------------------------------- caso 1
        if (root->left == nullptr && root->right == nullptr) {
            T.at(__LINE__, __func__)
             .vars({{"caso", "1 - foglia"}, {"key", key}})
             .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(nullptr)).set(root, "doomed")))
             .note("CASO 1 — foglia. " + std::to_string(key) + " non ha figli: il padre lo sostituisce "
                   "con nullptr e il nodo viene liberato. Nessun'altra sistemazione serve.")
             .emit();
            delete root;
            return nullptr;
        }

        // ---------------------------------------------------------- caso 2
        if (root->left == nullptr) {
            Node* temp = root->right;
            T.at(__LINE__, __func__)
             .vars({{"caso", "2 - un figlio"}, {"key", key}})
             .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(nullptr)).set(root, "doomed").set(temp, "moved")))
             .note("CASO 2 — un solo figlio (destro). Il figlio " + std::to_string(temp->data) +
                   " prende il posto del padre: tutto il suo sottoalbero è già dalla parte giusta "
                   "rispetto al nonno, quindi la proprietà del BST regge.")
             .emit();
            delete root;
            return temp;
        }
        if (root->right == nullptr) {
            Node* temp = root->left;
            T.at(__LINE__, __func__)
             .vars({{"caso", "2 - un figlio"}, {"key", key}})
             .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(nullptr)).set(root, "doomed").set(temp, "moved")))
             .note("CASO 2 — un solo figlio (sinistro). Il figlio " + std::to_string(temp->data) +
                   " sale al posto del padre.")
             .emit();
            delete root;
            return temp;
        }

        // ---------------------------------------------------------- caso 3
        T.at(__LINE__, __func__)
         .vars({{"caso", "3 - due figli"}, {"key", key}})
         .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(root)).set(root, "doomed")))
         .note("CASO 3 — due figli. Non si può semplicemente staccare " + std::to_string(key) +
               ": va sostituito con un valore che stia fra tutti i minori e tutti i maggiori. "
               "Quel valore è il successore in-order.")
         .emit();

        Node* successor = findMin(root->right);
        T.at(__LINE__, __func__)
         .vars({{"caso", "3 - due figli"}, {"successore", successor->data}})
         .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(root)).set(root, "doomed").set(successor, "moved")))
         .note("Successore trovato: " + std::to_string(successor->data) +
               ", il minimo del sottoalbero destro. È il più piccolo valore ancora maggiore di " +
               std::to_string(key) + ".")
         .emit();

        root->data = successor->data;
        T.at(__LINE__, __func__)
         .vars({{"caso", "3 - due figli"}, {"root->data", root->data}})
         .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(root)).set(successor, "doomed")))
         .note("Copio il valore del successore nel nodo: la posizione nell'albero non cambia, cambia "
               "solo il contenuto. Ora però il successore compare due volte.")
         .emit();

        path.push_back(root);
        root->right = deleteNode(root->right, successor->data);
        T.at(__LINE__, __func__)
         .vars({{"caso", "3 - due figli"}})
         .treeState(trace::binaryTreeJson(gRoot, marks(root)))
         .note("Il duplicato è stato cancellato dal sottoalbero destro. Quella cancellazione ricade "
               "sempre nel caso 1 o nel caso 2, perché il minimo non ha figlio sinistro: la "
               "ricorsione non può ripresentare il caso 3.")
         .emit();
    }
    return root;
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

    T.input(label);
    T.at(__LINE__, "deleteNode")
     .vars({{"key", key}})
     .treeState(trace::binaryTreeJson(root))
     .note(intro)
     .emit();

    root = deleteNode(root, key);
    gRoot = root;
    T.at(__LINE__, "deleteNode")
     .treeState(trace::binaryTreeJson(gRoot))
     .note("Albero finale. L'in-order è ancora crescente: la proprietà del BST è intatta.")
     .emit();
    freeTree(root);
}

int main() {
    T.view("tree").complexity("O(h)", "O(h)");

    run("caso 1 — foglia (cancella 1)", 1, "Cancello 1, che è una foglia. È il caso più semplice.");
    run("caso 2 — un figlio (cancella 14)", 14, "Cancello 14, che ha solo il figlio sinistro 13.");
    run("caso 3 — due figli (cancella 3)", 3, "Cancello 3, che ha due figli: 1 e 6.");
    run("caso 3 sulla radice (cancella 8)", 8, "Cancello la radice, che ha due figli.");
    run("chiave assente (cancella 5)", 5, "Cancello 5, che nell'albero non c'è.");

    T.dump("traces/bst_delete.js");
    return 0;
}
