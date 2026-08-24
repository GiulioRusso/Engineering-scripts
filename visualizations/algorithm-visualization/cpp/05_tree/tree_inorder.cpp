// In-order traversal (DFS)
//
// Left → root → right. On a binary search tree this order produces the
// values in ascending order, and it's the cheapest way to verify that a BST
// really is one.
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

void inorder(Node* node) {
    if (node == nullptr) {
        T.at(__LINE__, __func__)
         .treeState(trace::binaryTreeJson(gRoot, marks(nullptr)))
         .sequence(outputText())
         .note("Null child: the call returns right away. This is the base case that stops the recursion.")
         .emit();
        return;
    }

    T.pushFrame("inorder(" + std::to_string(node->data) + ")");
    T.at(__LINE__, __func__)
     .vars({{"node->data", node->data}})
     .treeState(trace::binaryTreeJson(gRoot, marks(node)))
     .sequence(outputText())
     .note("Entering node " + std::to_string(node->data) + ". Before printing it, its whole left "
           "subtree has to be finished.")
     .emit();

    inorder(node->left);

    output.push_back(node->data);
    visited.push_back(node);
    T.at(__LINE__, __func__)
     .vars({{"node->data", node->data}})
     .treeState(trace::binaryTreeJson(gRoot, marks(node)))
     .visit(node->data)
     .sequence(outputText())
     .counters({{"visited", (int)output.size()}})
     .note("Left subtree exhausted: printing " + std::to_string(node->data) + ".")
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
     .counters({{"visited", 0}})
     .note(intro)
     .emit();

    inorder(root);

    T.at(__LINE__, "inorder")
     .treeState(trace::binaryTreeJson(gRoot, marks(nullptr)))
     .sequence(outputText())
     .counters({{"visited", (int)output.size()}})
     .note("Traversal complete. On a BST the sequence is increasing: that's the property that makes "
           "in-order the natural correctness check for a search tree.")
     .emit();
    freeTree(root);
}

int main() {
    T.view("tree").complexity("O(n)", "O(h)");
    T.panel("callstack", "Call stack");

    trace::resetNodeIds();
    run("book tree", buildBook(), "Binary search tree. In-order: left, root, right.");
    trace::resetNodeIds();
    run("small tree", buildSmall(), "A smaller tree, to follow the recursion without getting lost.");

    T.dump("traces/tree_inorder.js");
    return 0;
}
