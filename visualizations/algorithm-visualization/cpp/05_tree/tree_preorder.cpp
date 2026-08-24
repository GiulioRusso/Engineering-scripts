// Pre-order traversal (DFS)
//
// Root → left → right. The node is emitted as soon as it's reached, before
// descending: it's the right order to copy or serialize a tree, because
// rebuilding it in the same order always finds the parent already created.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("tree_preorder", "Pre-order (DFS)", "Tree", __FILE__);

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

static Node* gRoot = nullptr;   // current root: needed to always draw the whole tree
static std::vector<int> output;
static std::vector<Node*> visited;

static std::string outputText() {
    std::string s;
    for (size_t i = 0; i < output.size(); ++i) s += (i ? " " : "") + std::to_string(output[i]);
    return "output: " + (s.empty() ? "(empty)" : s);
}

static trace::TreeMarks marks(Node* current) {
    trace::TreeMarks m;
    for (Node* v : visited) m.set(v, "visited");
    m.set(current, "current");
    return m;
}

void preorder(Node* node) {
    if (node == nullptr) {
        T.at(__LINE__, __func__)
         .treeState(trace::binaryTreeJson(gRoot, marks(nullptr)))
         .sequence(outputText())
         .note("Null child: the call returns right away. This is the base case that stops the recursion.")
         .emit();
        return;
    }

    T.pushFrame("preorder(" + std::to_string(node->data) + ")");
    T.at(__LINE__, __func__)
     .vars({{"node->data", node->data}})
     .treeState(trace::binaryTreeJson(gRoot, marks(node)))
     .sequence(outputText())
     .note("Entering node " + std::to_string(node->data) + ". In pre-order the node is emitted "
           "before any descent.")
     .emit();

    output.push_back(node->data);
    visited.push_back(node);
    T.at(__LINE__, __func__)
     .vars({{"node->data", node->data}})
     .treeState(trace::binaryTreeJson(gRoot, marks(node)))
     .visit(node->data)
     .sequence(outputText())
     .counters({{"visited", (int)output.size()}})
     .note("Printing " + std::to_string(node->data) + " right away, as soon as the node is entered: then descending.")
     .emit();

    preorder(node->left);

    preorder(node->right);
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
    T.at(__LINE__, "preorder")
     .treeState(trace::binaryTreeJson(root))
     .sequence(outputText())
     .counters({{"visited", 0}})
     .note(intro)
     .emit();

    preorder(root);

    T.at(__LINE__, "preorder")
     .treeState(trace::binaryTreeJson(gRoot, marks(nullptr)))
     .sequence(outputText())
     .counters({{"visited", (int)output.size()}})
     .note("Traversal complete. Rebuilding the tree by inserting the values in this order gives back "
           "exactly the same tree.")
     .emit();
    freeTree(root);
}

int main() {
    T.view("tree").complexity("O(n)", "O(h)");
    T.panel("callstack", "Call stack");

    trace::resetNodeIds();
    run("book tree", buildBook(), "Binary search tree. Pre-order: root, left, right.");
    trace::resetNodeIds();
    run("small tree", buildSmall(), "A smaller tree, to follow the recursion without getting lost.");

    T.dump("traces/tree_preorder.js");
    return 0;
}
