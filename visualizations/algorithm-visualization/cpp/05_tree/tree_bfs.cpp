// Breadth-First (Level-Order) traversal
//
// Visits the tree one level at a time. It isn't recursive: the structure
// standing in for the recursion is a queue, and it's the same circular queue
// from the previous chapter. Every node is enqueued when it's encountered and
// dequeued when it's visited, so it comes out in the order it went in.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("tree_bfs", "Breadth-First (Level-Order)", "Tree", __FILE__);

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

// The circular queue from the Stack and Queue chapter, reused as-is.
static const int QCAP = 8;
static Node* queueData[QCAP];
static int front = 0, rear = -1, count = 0;

void enqueue(Node* node) {
    rear = (rear + 1) % QCAP;
    queueData[rear] = node;
    count++;
}

Node* dequeue() {
    Node* node = queueData[front];
    queueData[front] = nullptr;
    front = (front + 1) % QCAP;
    count--;
    return node;
}

static Node* gRoot = nullptr;
static std::vector<int> output;
static std::vector<Node*> visited;

static std::string outputText() {
    std::string s;
    for (size_t i = 0; i < output.size(); ++i) s += (i ? " " : "") + std::to_string(output[i]);
    return "Visited: " + (s.empty() ? "(empty)" : s);
}

static trace::TreeMarks marks(Node* current) {
    trace::TreeMarks m;
    for (Node* v : visited) m.set(v, "visited");
    for (int i = 0; i < count; i++) m.set(queueData[(front + i) % QCAP], "path");
    m.set(current, "current");
    return m;
}

void bfsTraversal(Node* root) {
    if (root == nullptr) return;

    enqueue(root);
    T.at(__LINE__, __func__)
     .vars({{"front", front}, {"rear", rear}, {"count", count}})
     .treeState(trace::binaryTreeJson(gRoot, marks(nullptr)))
     .enqueue(root->data)
     .sequence(outputText())
     .note("Enqueuing the root. The queue always holds the frontier: nodes encountered but not yet visited.")
     .emit();

    while (count > 0) {
        Node* current = dequeue();
        output.push_back(current->data);
        visited.push_back(current);
        T.at(__LINE__, __func__)
         .vars({{"front", front}, {"rear", rear}, {"count", count}, {"current->data", current->data}})
         .treeState(trace::binaryTreeJson(gRoot, marks(current)))
         .visit(current->data)
         .sequence(outputText())
         .counters({{"visited", (int)output.size()}})
         .note("Dequeuing from the head and visiting " + std::to_string(current->data) +
               ". The Visited row fills up in level order.")
         .emit();

        if (current->left != nullptr) {
            enqueue(current->left);
            T.at(__LINE__, __func__)
             .vars({{"front", front}, {"rear", rear}, {"count", count}})
             .treeState(trace::binaryTreeJson(gRoot, marks(current)))
             .enqueue(current->left->data)
             .sequence(outputText())
             .note("Enqueuing the left child " + std::to_string(current->left->data) +
                   ": it will be visited after every node already in the queue, i.e. after the rest of this level.")
             .emit();
        }

        if (current->right != nullptr) {
            enqueue(current->right);
            T.at(__LINE__, __func__)
             .vars({{"front", front}, {"rear", rear}, {"count", count}})
             .treeState(trace::binaryTreeJson(gRoot, marks(current)))
             .enqueue(current->right->data)
             .sequence(outputText())
             .note("Enqueuing the right child " + std::to_string(current->right->data) + ".")
             .emit();
        }
    }

    T.at(__LINE__, __func__)
     .vars({{"front", front}, {"rear", rear}, {"count", count}})
     .treeState(trace::binaryTreeJson(gRoot, marks(nullptr)))
     .sequence(outputText())
     .counters({{"visited", (int)output.size()}})
     .note("Empty queue: no more frontier, the tree has been visited level by level.")
     .emit();
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

static Node* buildFull() {
    Node* root = createNode(1);
    root->left = createNode(2);
    root->right = createNode(3);
    root->left->left = createNode(4);
    root->left->right = createNode(5);
    root->right->left = createNode(6);
    root->right->right = createNode(7);
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
    for (int i = 0; i < QCAP; i++) queueData[i] = nullptr;
    front = 0;
    rear = -1;
    count = 0;

    T.input(label);
    T.watch("queue", [] {
        std::string s = "[";
        for (int i = 0; i < QCAP; i++) {
            if (i) s += ",";
            s += queueData[i] ? std::to_string(queueData[i]->data) : "0";
        }
        return s + "]";
    });
    T.at(__LINE__, "bfsTraversal")
     .vars({{"front", front}, {"rear", rear}, {"count", count}})
     .treeState(trace::binaryTreeJson(root))
     .sequence(outputText())
     .counters({{"visited", 0}})
     .note(intro)
     .emit();

    bfsTraversal(root);
    freeTree(root);
}

int main() {
    T.view("tree").complexity("O(n)", "O(w) with w = maximum width");
    T.panel("queue", "Queue of nodes to visit", "queue");

    trace::resetNodeIds();
    run("book tree", buildBook(), "The book's tree. The queue starts empty: front = 0, rear = -1.");
    trace::resetNodeIds();
    run("full tree", buildFull(), "Full tree of 7 nodes: the queue ends up holding an entire level.");

    T.dump("traces/tree_bfs.js");
    return 0;
}
