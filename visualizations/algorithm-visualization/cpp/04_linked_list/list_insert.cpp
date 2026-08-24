// Linked List: Insert (head / tail / position)
//
// Insertion is O(1) if you're already holding the previous node: you
// allocate and re-hook two pointers, no data ever moves. The cost, when
// there is one, is entirely in reaching that previous node.
//
// The book only implements head and tail: insert-at-position is an
// addition.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("list_insert", "Insert (head / tail / position)", "Linked List", __FILE__);

struct Node {
    int data;
    Node* next;
};

Node* createNode(int value) {
    Node* newNode = new Node;
    newNode->data = value;
    newNode->next = nullptr;
    return newNode;
}

void insertAtHead(Node*& head, int value) {
    Node* newNode = createNode(value);
    T.at(__LINE__, __func__)
     .vars({{"value", value}})
     .listState(trace::listJson<Node>(head, "singly", nullptr, {{newNode, "new", 0}}))
     .note("Node allocated outside the chain: its next is NULL and nothing reaches it yet.")
     .emit();

    newNode->next = head;
    T.at(__LINE__, __func__)
     .vars({{"value", value}})
     .listState(trace::listJson<Node>(head, "singly", nullptr, {{newNode, "new", 0}}))
     .link(trace::nodeId(newNode), trace::nodeId(newNode->next))
     .note("First hook: newNode->next points to the old head. The list is still reachable from head.")
     .emit();

    head = newNode;
    T.at(__LINE__, __func__)
     .vars({{"value", value}})
     .listState(trace::listJson(head, "singly", newNode))
     .link("head", trace::nodeId(newNode))
     .note("Second hook: head moves to the new node. Insert at the head: O(1), no scan.")
     .emit();
}

void insertAtTail(Node*& head, int value) {
    Node* newNode = createNode(value);
    if (head == nullptr) {
        head = newNode;
        T.at(__LINE__, __func__)
         .vars({{"value", value}})
         .listState(trace::listJson(head, "singly", newNode))
         .note("Empty list: the new node becomes the head directly.")
         .emit();
        return;
    }

    Node* current = head;
    while (current->next != nullptr) {
        current = current->next;
        T.at(__LINE__, __func__)
         .vars({{"value", value}, {"current->data", current->data}})
         .listState(trace::listJson(head, "singly", current, {{newNode, "new", -1}}))
         .note("Looking for the tail by following next. This scan is what makes insert-at-tail O(n).")
         .emit();
    }

    current->next = newNode;
    T.at(__LINE__, __func__)
     .vars({{"value", value}})
     .listState(trace::listJson(head, "singly", newNode))
     .link(trace::nodeId(current), trace::nodeId(newNode))
     .note("Found the tail (next == NULL): hooking it to the new node, which becomes the new tail.")
     .emit();
}

void insertAtPosition(Node*& head, int value, int position) {
    if (position == 0) {
        insertAtHead(head, value);
        return;
    }

    Node* current = head;
    for (int i = 0; i < position - 1 && current != nullptr; i++) {
        current = current->next;
        T.at(__LINE__, __func__)
         .vars({{"position", position}, {"i", i + 1}})
         .listState(trace::listJson(head, "singly", current))
         .note("Advancing to the node right before position " + std::to_string(position) + ".")
         .emit();
    }

    if (current == nullptr) {
        T.at(__LINE__, __func__)
         .vars({{"position", position}})
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .error("out_of_range")
         .note("The list ends before position " + std::to_string(position) +
               ": there is no previous node to hook onto.")
         .emit();
        return;
    }

    Node* newNode = createNode(value);
    T.at(__LINE__, __func__)
     .vars({{"position", position}, {"value", value}})
     .listState(trace::listJson(head, "singly", current, {{newNode, "new", position}}))
     .note("New node allocated above the insertion point, still disconnected.")
     .emit();

    newNode->next = current->next;
    T.at(__LINE__, __func__)
     .vars({{"position", position}, {"value", value}})
     .listState(trace::listJson(head, "singly", current, {{newNode, "new", position}}))
     .link(trace::nodeId(newNode), trace::nodeId(newNode->next))
     .note("FIRST: newNode->next takes the rest of the list. If the order of the two hooks were "
           "reversed, current->next would get overwritten and the whole tail of the list would be lost.")
     .emit();

    current->next = newNode;
    T.at(__LINE__, __func__)
     .vars({{"position", position}, {"value", value}})
     .listState(trace::listJson(head, "singly", newNode))
     .link(trace::nodeId(current), trace::nodeId(newNode))
     .note("SECOND: current->next points to the new node. The chain is whole again and holds "
           "one more element.")
     .emit();
}

static Node* buildList(const int* values, int n) {
    Node* head = nullptr;
    Node* tail = nullptr;
    for (int i = 0; i < n; i++) {
        Node* node = createNode(values[i]);
        if (!head) { head = tail = node; }
        else { tail->next = node; tail = node; }
    }
    return head;
}

static void freeList(Node* head) {
    while (head) { Node* next = head->next; delete head; head = next; }
}

enum Mode { HEAD, TAIL, POS };

static void run(const char* label, Mode mode, int value, int position, const char* intro) {
    trace::resetNodeIds();
    const int values[] = {10, 20, 30, 40};
    Node* head = buildList(values, 4);

    T.input(label);
    T.at(__LINE__, "main")
     .listState(trace::listJson<Node>(head, "singly", nullptr))
     .note(intro)
     .emit();

    if (mode == HEAD) insertAtHead(head, value);
    else if (mode == TAIL) insertAtTail(head, value);
    else insertAtPosition(head, value, position);

    freeList(head);
}

int main() {
    T.view("linkedlist").complexity("O(1) at the head, O(n) at the tail or a position", "O(1)");

    run("at the head (O(1))", HEAD, 5, 0, "List of 4 nodes. Inserting 5 at the head.");
    run("at the tail (O(n))", TAIL, 50, 0, "List of 4 nodes. Inserting 50 at the tail: it has to be found first.");
    run("at position 2", POS, 25, 2, "List of 4 nodes. Inserting 25 at position 2, between 20 and 30.");
    run("at position 0 (falls back to the head)", POS, 5, 0, "Position 0: the case reduces to insert-at-head.");
    run("position past the end", POS, 99, 9, "Position 9 on a list of 4 nodes: it has to fail.");

    T.dump("traces/list_insert.js");
    return 0;
}
