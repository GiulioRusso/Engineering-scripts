// BST: Search
//
// At every node, one question, and the answer throws away half the work: if
// the key is smaller you go left and the whole right subtree is never looked
// at. The cost is the tree's height, O(h), not the number of nodes.
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
static std::vector<Node*> path;      // nodes already visited
static std::vector<Node*> pruned;    // roots of the discarded subtrees
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
         .counters({{"comparisons", ++comparisons}})
         .note("At node " + std::to_string(current->data) + ". Comparing with key " +
               std::to_string(key) + ": one question, three possible answers.")
         .emit();

        if (key == current->data) {
            T.at(__LINE__, __func__)
             .vars({{"key", key}})
             .treeState(trace::binaryTreeJson(gRoot, trace::TreeMarks(marks(nullptr)).set(current, "found")))
             .found(0)
             .counters({{"comparisons", comparisons}})
             .note("Found: " + std::to_string(key) + " was at depth " + std::to_string(path.size()) +
                   ", reached with " + std::to_string(comparisons) + " comparisons.")
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
             .counters({{"comparisons", comparisons}})
             .note(std::to_string(key) + " is smaller: going left. The whole right subtree, "
                   "in grey, will never be examined — that's the saving.")
             .emit();
        } else {
            if (current->left) pruned.push_back(current->left);
            current = current->right;
            T.at(__LINE__, __func__)
             .vars({{"key", key}})
             .treeState(trace::binaryTreeJson(gRoot, marks(current)))
             .counters({{"comparisons", comparisons}})
             .note(std::to_string(key) + " is larger: going right, and the left subtree drops "
                   "out of the picture.")
             .emit();
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"key", key}})
     .treeState(trace::binaryTreeJson(gRoot, marks(nullptr)))
     .counters({{"comparisons", comparisons}})
     .note("current is NULL: I've fallen out of the tree, so " + std::to_string(key) +
           " isn't there. Returning nullptr.")
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
     .counters({{"comparisons", 0}})
     .note(intro)
     .emit();

    search(root, key);
    freeTree(root);
}

int main() {
    T.view("tree").complexity("O(h), O(log n) if balanced", "O(1)");

    run("search for 4 (book's example)", 4,
        "The book's tree, 9 nodes. Searching for 4: only a few comparisons needed, not nine.");
    run("search for 13 (to the right)", 13, "Searching for 13: the descent goes all the way right.");
    run("search for 8 (the root)", 8, "Searching for the root: a single comparison, the best case.");
    run("search for 5 (absent)", 5, "Searching for 5, which isn't there: the search falls onto a null pointer.");

    T.dump("traces/bst_search.js");
    return 0;
}
