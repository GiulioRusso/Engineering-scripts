// BST: Delete (the three cases)
//
// This is the most delicate piece of the chapter, because deletion has to
// leave the BST property intact. Three cases, and the trace always states
// which one is being applied:
//
//   1. leaf            -> just detach it
//   2. one child only   -> the child takes the parent's place
//   3. two children     -> copy the in-order successor (the minimum of the
//                          right subtree) in, then delete that one, which by
//                          construction falls into case 1 or case 2
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("bst_delete", "Delete (3 cases)", "Binary Search Tree", __FILE__);

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

// The minimum of a subtree is its leftmost node: it has no left child,
// otherwise it wouldn't be the minimum.
Node* findMin(Node* node) {
    while (node->left != nullptr) {
        node = node->left;
        T.at(__LINE__, __func__)
         .vars({{"node->data", node->data}})
         .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(node)).set(node, "moved")))
         .note("Going further left: the in-order successor is the leftmost node of the "
               "right subtree.")
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
         .note("Null pointer: key " + std::to_string(key) + " is not in the tree.")
         .emit();
        return nullptr;
    }

    T.at(__LINE__, __func__)
     .vars({{"key", key}, {"root->data", root->data}})
     .treeState(trace::binaryTreeJson(gRoot, marks(root)))
     .compare(0, -1)
     .note("Looking for " + std::to_string(key) + ", at node " + std::to_string(root->data) + ".")
     .emit();

    if (key < root->data) {
        path.push_back(root);
        root->left = deleteNode(root->left, key);
    } else if (key > root->data) {
        path.push_back(root);
        root->right = deleteNode(root->right, key);
    } else {
        // ---------------------------------------------------------- case 1
        if (root->left == nullptr && root->right == nullptr) {
            T.at(__LINE__, __func__)
             .vars({{"case", "1 - leaf"}, {"key", key}})
             .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(nullptr)).set(root, "doomed")))
             .note("CASE 1 — leaf. " + std::to_string(key) + " has no children: the parent replaces "
                   "it with nullptr and the node is freed. Nothing else needs adjusting.")
             .emit();
            delete root;
            return nullptr;
        }

        // ---------------------------------------------------------- case 2
        if (root->left == nullptr) {
            Node* temp = root->right;
            T.at(__LINE__, __func__)
             .vars({{"case", "2 - one child"}, {"key", key}})
             .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(nullptr)).set(root, "doomed").set(temp, "moved")))
             .note("CASE 2 — one child only (right). Child " + std::to_string(temp->data) +
                   " takes the parent's place: its whole subtree is already on the correct side "
                   "of the grandparent, so the BST property holds.")
             .emit();
            delete root;
            return temp;
        }
        if (root->right == nullptr) {
            Node* temp = root->left;
            T.at(__LINE__, __func__)
             .vars({{"case", "2 - one child"}, {"key", key}})
             .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(nullptr)).set(root, "doomed").set(temp, "moved")))
             .note("CASE 2 — one child only (left). Child " + std::to_string(temp->data) +
                   " moves up into the parent's place.")
             .emit();
            delete root;
            return temp;
        }

        // ---------------------------------------------------------- case 3
        T.at(__LINE__, __func__)
         .vars({{"case", "3 - two children"}, {"key", key}})
         .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(root)).set(root, "doomed")))
         .note("CASE 3 — two children. " + std::to_string(key) + " can't just be detached: it has to "
               "be replaced with a value that sits between everything smaller and everything larger. "
               "That value is the in-order successor.")
         .emit();

        Node* successor = findMin(root->right);
        T.at(__LINE__, __func__)
         .vars({{"case", "3 - two children"}, {"successor", successor->data}})
         .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(root)).set(root, "doomed").set(successor, "moved")))
         .note("Successor found: " + std::to_string(successor->data) +
               ", the minimum of the right subtree. It's the smallest value still greater than " +
               std::to_string(key) + ".")
         .emit();

        root->data = successor->data;
        T.at(__LINE__, __func__)
         .vars({{"case", "3 - two children"}, {"root->data", root->data}})
         .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(root)).set(successor, "doomed")))
         .note("Copying the successor's value into the node: its position in the tree doesn't change, "
               "only the content does. But now the successor's value appears twice.")
         .emit();

        path.push_back(root);
        root->right = deleteNode(root->right, successor->data);
        T.at(__LINE__, __func__)
         .vars({{"case", "3 - two children"}})
         .treeState(trace::binaryTreeJson(gRoot, marks(root)))
         .note("The duplicate has been deleted from the right subtree. That deletion always falls "
               "into case 1 or case 2, because the minimum has no left child: the recursion can "
               "never hit case 3 again.")
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
     .note("Final tree. The in-order traversal is still increasing: the BST property is intact.")
     .emit();
    freeTree(root);
}

int main() {
    T.view("tree").complexity("O(h)", "O(h)");

    run("case 1 - leaf (delete 1)", 1, "Deleting 1, which is a leaf. The simplest case.");
    run("case 2 - one child (delete 14)", 14, "Deleting 14, which has only a left child, 13.");
    run("case 3 - two children (delete 3)", 3, "Deleting 3, which has two children: 1 and 6.");
    run("case 3 on the root (delete 8)", 8, "Deleting the root, which has two children.");
    run("absent key (delete 5)", 5, "Deleting 5, which isn't in the tree.");

    T.dump("traces/bst_delete.js");
    return 0;
}
