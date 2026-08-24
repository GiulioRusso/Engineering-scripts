// N-ary Tree: addChild, display, deleteTree
//
// A generic tree has no two-child constraint: each node holds a vector of
// children, and the number of children is its degree. The root is the only
// node with no parent, leaves are the nodes with no children, a node's depth
// is its distance from the root, and the tree's height is the maximum depth.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("tree_basics", "N-ary Tree: addChild, display, deleteTree", "Tree", __FILE__);

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
    return "output: " + (s.empty() ? "(empty)" : s);
}

static trace::TreeMarks labelled() {
    trace::TreeMarks m;
    m.tag(gRoot, "root");
    return m;
}

// display: pre-order over an n-ary tree. The node is printed as soon as it's
// visited, then descends into all children left to right.
void display(Node* node, int depth) {
    if (node == nullptr) return;

    output.push_back(node->data);
    T.pushFrame("display(" + std::to_string(node->data) + ", depth=" + std::to_string(depth) + ")");
    T.at(__LINE__, __func__)
     .vars({{"node->data", node->data}, {"depth", depth}, {"degree", (int)node->children.size()}})
     .treeState(trace::naryTreeJson(gRoot, trace::TreeMarks(labelled()).set(node, "current")))
     .visit(node->data)
     .sequence(outputText())
     .counters({{"visited", (int)output.size()}})
     .note("Node " + std::to_string(node->data) + ": depth " + std::to_string(depth) + ", degree " +
           std::to_string(node->children.size()) +
           (node->children.empty() ? ". It's a leaf." : ". Descending into the children, left to right."))
     .emit();

    for (Node* child : node->children) {
        display(child, depth + 1);
    }
    T.popFrame();
}

// deleteTree: post-order. The children must be freed BEFORE the parent,
// otherwise once the parent is deleted the children vector no longer exists
// and the children become unreachable. Each pointer is zeroed as soon as the
// child is freed: in the animation you see the subtree disappear.
void deleteTree(Node* node) {
    if (node == nullptr) return;

    T.pushFrame("deleteTree(" + std::to_string(node->data) + ")");
    T.at(__LINE__, __func__)
     .vars({{"node->data", node->data}, {"children", (int)node->children.size()}})
     .treeState(trace::naryTreeJson(gRoot, trace::TreeMarks(labelled()).set(node, "current")))
     .note("Entering " + std::to_string(node->data) +
           " but not freeing it yet: all of its children have to disappear first.")
     .emit();

    for (size_t i = 0; i < node->children.size(); i++) {
        deleteTree(node->children[i]);
        node->children[i] = nullptr;   // no dangling pointers after the delete
    }

    T.at(__LINE__, __func__)
     .vars({{"node->data", node->data}})
     .treeState(trace::naryTreeJson(gRoot, trace::TreeMarks(labelled()).set(node, "doomed")))
     .note("The children of " + std::to_string(node->data) + " have been freed: now it's its turn. "
           "This is the post-order, and it's the only safe order to destroy a tree in.")
     .emit();
    T.popFrame();
    delete node;
}

int main() {
    T.view("tree").complexity("O(n)", "O(h)");
    T.panel("callstack", "Call stack");

    // ------------------------------------------------ building with addChild
    trace::resetNodeIds();
    Node* root = createNode(1);
    gRoot = root;
    output.clear();

    T.input("building with addChild");
    T.at(__LINE__, "addChild")
     .vars({{"height", height(root)}, {"nodes", 1}})
     .treeState(trace::naryTreeJson(gRoot, labelled()))
     .note("A single node: it's the root and a leaf at once. Height 0, degree 0.")
     .emit();

    const int level1[] = {2, 3, 4};
    for (int value : level1) {
        addChild(root, createNode(value));
        T.at(__LINE__, "addChild")
         .vars({{"height", height(root)}, {"root degree", (int)root->children.size()}})
         .treeState(trace::naryTreeJson(gRoot, trace::TreeMarks(labelled()).set(root->children.back(), "found")))
         .note("addChild appends " + std::to_string(value) +
               " to the end of the root's children vector: no limit of two, the degree rises to " +
               std::to_string(root->children.size()) + ".")
         .emit();
    }

    const int level2[] = {5, 6};
    for (int value : level2) {
        addChild(root->children[0], createNode(value));
        T.at(__LINE__, "addChild")
         .vars({{"height", height(root)}})
         .treeState(trace::naryTreeJson(gRoot, trace::TreeMarks(labelled()).set(root->children[0]->children.back(), "found")))
         .note("Adding " + std::to_string(value) + " under node 2: the tree's height rises to " +
               std::to_string(height(root)) + ", because that's the deepest leaf's depth.")
         .emit();
    }

    // ---------------------------------------------------------------- display
    T.input("display (n-ary pre-order)");
    T.clearFrames();
    output.clear();
    T.at(__LINE__, "display")
     .vars({{"height", height(root)}})
     .treeState(trace::naryTreeJson(gRoot, labelled()))
     .sequence(outputText())
     .counters({{"visited", 0}})
     .note("The tree built above. display walks it in pre-order.")
     .emit();
    display(root, 0);

    // ------------------------------------------------------------- deleteTree
    T.input("deleteTree (post-order)");
    T.clearFrames();
    T.at(__LINE__, "deleteTree")
     .treeState(trace::naryTreeJson(gRoot, labelled()))
     .note("Destroying the tree. Watch the order: no node disappears before its children.")
     .emit();
    deleteTree(root);
    gRoot = nullptr;
    T.at(__LINE__, "deleteTree")
     .treeState(trace::naryTreeJson<Node>(nullptr))
     .note("Empty tree: all the memory has been freed, from the leaves toward the root.")
     .emit();

    T.dump("traces/tree_basics.js");
    return 0;
}
