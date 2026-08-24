// Singly / Doubly / Circular
//
// Not an algorithm but a comparison: the same operations on the three
// variants, to see what changes in the pointers and what that makes
// possible. The book defines them without implementing them: here they are
// implemented.
//
//   singly   : one next only, the tail points to NULL, no going back
//   doubly   : also prev, walkable in both directions, one extra pointer per node
//   circular : the tail points to the head, there is no NULL to stop at
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("list_types", "Singly / Doubly / Circular", "Linked List", __FILE__);

struct Node {
    int data;
    Node* next;
    Node* prev;   // used only by the doubly linked list
};

Node* createNode(int value) {
    Node* newNode = new Node;
    newNode->data = value;
    newNode->next = nullptr;
    newNode->prev = nullptr;
    return newNode;
}

// In the singly linked list the tail stays with next = NULL.
Node* buildSingly(const int* values, int n) {
    Node* head = nullptr;
    Node* tail = nullptr;
    for (int i = 0; i < n; i++) {
        Node* node = createNode(values[i]);
        if (!head) head = tail = node;
        else { tail->next = node; tail = node; }
    }
    return head;
}

// In the doubly linked list every node also remembers the previous one.
Node* buildDoubly(const int* values, int n) {
    Node* head = nullptr;
    Node* tail = nullptr;
    for (int i = 0; i < n; i++) {
        Node* node = createNode(values[i]);
        if (!head) head = tail = node;
        else { tail->next = node; node->prev = tail; tail = node; }
    }
    return head;
}

// In the circular list the tail points to the head: no NULL anywhere.
Node* buildCircular(const int* values, int n) {
    Node* head = nullptr;
    Node* tail = nullptr;
    for (int i = 0; i < n; i++) {
        Node* node = createNode(values[i]);
        if (!head) head = tail = node;
        else { tail->next = node; tail = node; }
    }
    if (tail) tail->next = head;
    return head;
}

static Node* singly = nullptr;
static Node* doubly = nullptr;
static Node* circular = nullptr;

static void snapshot(int line, const char* fn, const std::string& note, Node* cursor = nullptr) {
    T.at(line, fn)
     .listState(trace::listJson(singly, "singly", cursor))
     .listState("doubly", trace::listJson(doubly, "doubly", (Node*)nullptr))
     .listState("circular", trace::listJson(circular, "circular", (Node*)nullptr))
     .note(note)
     .emit();
}

// Insert-at-head on the three variants: what changes is how many pointers
// need fixing, not the cost, which stays O(1) everywhere.
void insertHeadAll(int value) {
    Node* a = createNode(value);
    a->next = singly;
    singly = a;

    Node* b = createNode(value);
    b->next = doubly;
    if (doubly) doubly->prev = b;
    doubly = b;

    Node* c = createNode(value);
    if (circular) {
        Node* tail = circular;
        while (tail->next != circular) tail = tail->next;
        c->next = circular;
        tail->next = c;
    } else {
        c->next = c;
    }
    circular = c;

    T.at(__LINE__, __func__)
     .vars({{"value", value}})
     .listState(trace::listJson(singly, "singly", singly))
     .listState("doubly", trace::listJson(doubly, "doubly", doubly))
     .listState("circular", trace::listJson(circular, "circular", circular))
     .link("head", trace::nodeId(a))
     .note("Insert at head. Singly: one hook. Doubly: two, because the old first node also has to "
           "remember the new one (prev). Circular: two, but one requires finding the tail, and finding "
           "it costs O(n) if it isn't kept on hand.")
     .emit();
}

int main() {
    T.view("linkedlist").complexity("O(1) insert at head", "O(n)");
    T.panel("linkedlist", "Doubly linked", "doubly");
    T.panel("linkedlist", "Circular", "circular");

    const int values[] = {10, 20, 30, 40};

    trace::resetNodeIds();
    singly = buildSingly(values, 4);
    doubly = buildDoubly(values, 4);
    circular = buildCircular(values, 4);

    T.input("pointer structure compared");
    snapshot(__LINE__, "main",
             "The same four cells, three different structures. On top the singly linked list: the last "
             "next is NULL. In the first panel the doubly linked one: below the chain you can see the "
             "prevs. In the second the circular one: the tail loops back to the head, and there is no "
             "NULL anywhere.");
    snapshot(__LINE__, "main",
             "Practical consequence: in the singly linked list, deleting the tail means scanning all "
             "the way to the second-to-last node. In the doubly linked one you reach the tail and read "
             "its prev. In the circular one there is no end condition: the loop has to be stopped by "
             "counting, or by comparing against head.",
             singly);
    insertHeadAll(5);

    T.input("traversal");
    trace::resetNodeIds();
    singly = buildSingly(values, 4);
    doubly = buildDoubly(values, 4);
    circular = buildCircular(values, 4);

    Node* p = singly;
    int steps = 0;
    while (p != nullptr) {
        snapshot(__LINE__, "main",
                 "Singly: advancing until NULL is found. Step " + std::to_string(++steps) +
                 ". In the circular list the same condition would never terminate: the cursor would "
                 "come back to the head and start over.", p);
        p = p->next;
    }
    snapshot(__LINE__, "main",
             "p is NULL: the singly linked list's traversal ends here. For the circular one the right "
             "condition is `do { ... } while (p != head)`.");

    T.dump("traces/list_types.js");
    return 0;
}
