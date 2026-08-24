// BST: Insertion
//
// Insertion is the same descent as search: look for the key, and wherever the
// search would fail — a null pointer — hook the new node on. It follows that
// a new node is always a leaf, and that the insertion order determines the
// tree's shape.
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
         .counters({{"comparisons", comparisons}})
         .note("Null pointer: this is the free slot where the search would have failed. The new "
               "node lands here, and is always born as a leaf.")
         .emit();
        T.popFrame();
        return fresh;
    }

    T.at(__LINE__, __func__)
     .vars({{"value", value}, {"root->data", root->data}})
     .treeState(trace::binaryTreeJson(gRoot, marks(root)))
     .compare(0, -1)
     .counters({{"comparisons", ++comparisons}})
     .note("Comparing " + std::to_string(value) + " with " + std::to_string(root->data) +
           ": same decision as search.")
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
         .counters({{"comparisons", comparisons}})
         .note("Value already present: a classic BST doesn't allow duplicates, so the insertion does nothing.")
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
     .counters({{"comparisons", 0}})
     .note(intro)
     .emit();

    for (int v : values) {
        path.clear();
        T.clearFrames();
        T.at(__LINE__, "insert")
         .vars({{"value", v}})
         .treeState(trace::binaryTreeJson(gRoot))
         .counters({{"comparisons", comparisons}})
         .note("Inserting " + std::to_string(v) + ": starting from the root.")
         .emit();
        root = insert(root, v);
        gRoot = root;
    }

    path.clear();
    T.at(__LINE__, "insert")
     .treeState(trace::binaryTreeJson(gRoot))
     .counters({{"comparisons", comparisons}})
     .note("Tree complete. Its shape depends entirely on the order the values arrived in.")
     .emit();
    freeTree(root);
}

int main() {
    T.view("tree").complexity("O(h), O(log n) if balanced", "O(h)");
    T.panel("callstack", "Call stack");

    run("the book's tree", {8, 3, 10, 1, 6, 14, 4, 7, 13},
        "Empty tree. Inserting the book's values, one at a time.");
    run("inserting a duplicate", {8, 3, 10, 3},
        "The last two insertions are the same value: the second one must change nothing.");
    run("already-sorted values (degenerate tree)", {1, 2, 3, 4, 5, 6},
        "Increasing values: every node ends up to the right of the previous one and the tree "
        "degenerates into a list. Height n instead of log n, and search goes back to O(n).");

    T.dump("traces/bst_insert.js");
    return 0;
}
