// Linked List: Delete (head / tail / value / position)
//
// Deleting means having the node's predecessor jump over it, then freeing it.
// Between those two steps the node still exists but is no longer reachable:
// it's an orphan, and forgetting the delete would be a memory leak.
//
// The book only implements delete-by-value: delete-by-position is an
// addition.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("list_delete", "Delete (head / tail / value / position)", "Linked List", __FILE__);

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

void deleteAtHead(Node*& head) {
    if (head == nullptr) {
        T.at(__LINE__, __func__)
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .error("underflow")
         .note("Empty list: there is no head to delete.")
         .emit();
        return;
    }

    Node* temp = head;
    head = head->next;
    T.at(__LINE__, __func__)
     .vars({{"value", temp->data}})
     .listState(trace::listJson<Node>(head, "singly", nullptr, {{temp, "orphan", 0}}))
     .unlink("head", trace::nodeId(temp))
     .note("head moves to the second node. The old first node still exists in memory but is no longer "
           "reachable: it's an orphan.")
     .emit();

    delete temp;
    T.at(__LINE__, __func__)
     .listState(trace::listJson<Node>(head, "singly", nullptr))
     .note("delete frees the orphan's memory. Delete at the head: O(1).")
     .emit();
}

void deleteAtTail(Node*& head) {
    if (head == nullptr) {
        T.at(__LINE__, __func__)
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .error("underflow")
         .note("Empty list: nothing to delete.")
         .emit();
        return;
    }
    if (head->next == nullptr) {
        Node* temp = head;
        head = nullptr;
        delete temp;
        T.at(__LINE__, __func__)
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .note("Single node: the list becomes empty.")
         .emit();
        return;
    }

    Node* current = head;
    while (current->next->next != nullptr) {
        current = current->next;
        T.at(__LINE__, __func__)
         .vars({{"current->data", current->data}})
         .listState(trace::listJson(head, "singly", current))
         .note("Looking for the second-to-last node: in a singly linked list you can't go back from "
               "the tail, and that's exactly the problem the doubly linked list solves.")
         .emit();
    }

    Node* temp = current->next;
    current->next = nullptr;
    T.at(__LINE__, __func__)
     .vars({{"value", temp->data}})
     .listState(trace::listJson(head, "singly", current, {{temp, "orphan", -1}}))
     .unlink(trace::nodeId(current), trace::nodeId(temp))
     .note("The second-to-last node becomes the new tail (next = NULL) and the last one is left orphaned.")
     .emit();

    delete temp;
    T.at(__LINE__, __func__)
     .listState(trace::listJson<Node>(head, "singly", nullptr))
     .note("Orphan freed. The whole cost is in the scan: O(n).")
     .emit();
}

void deleteByValue(Node*& head, int value) {
    if (head == nullptr) {
        T.at(__LINE__, __func__)
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .error("underflow")
         .note("Empty list.")
         .emit();
        return;
    }
    if (head->data == value) {
        deleteAtHead(head);
        return;
    }

    Node* current = head;
    while (current->next != nullptr && current->next->data != value) {
        T.at(__LINE__, __func__)
         .vars({{"value", value}, {"current->data", current->data}, {"current->next->data", current->next->data}})
         .listState(trace::listJson(head, "singly", current))
         .note("Checking the value of the *next* node: to skip over it, you need to be holding its "
               "predecessor.")
         .emit();
        current = current->next;
    }

    if (current->next == nullptr) {
        T.at(__LINE__, __func__)
         .vars({{"value", value}})
         .listState(trace::listJson(head, "singly", current))
         .error("not_found")
         .note("Reached the end without finding " + std::to_string(value) + ": there is nothing to delete.")
         .emit();
        return;
    }

    Node* temp = current->next;
    current->next = temp->next;
    T.at(__LINE__, __func__)
     .vars({{"value", value}})
     .listState(trace::listJson(head, "singly", current, {{temp, "orphan", -1}}))
     .unlink(trace::nodeId(current), trace::nodeId(temp))
     .note("Bypass pointer: current->next skips the node to delete and goes straight to the one after "
           "it. The bypassed node is now an orphan.")
     .emit();

    delete temp;
    T.at(__LINE__, __func__)
     .listState(trace::listJson(head, "singly", current))
     .note("delete frees the orphan: the list is whole again, with one fewer node.")
     .emit();
}

void deleteAtPosition(Node*& head, int position) {
    if (position == 0) {
        deleteAtHead(head);
        return;
    }

    Node* current = head;
    for (int i = 0; i < position - 1 && current != nullptr; i++) {
        current = current->next;
        T.at(__LINE__, __func__)
         .vars({{"position", position}, {"i", i + 1}})
         .listState(trace::listJson(head, "singly", current))
         .note("Advancing to the predecessor of position " + std::to_string(position) + ".")
         .emit();
    }

    if (current == nullptr || current->next == nullptr) {
        T.at(__LINE__, __func__)
         .vars({{"position", position}})
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .error("out_of_range")
         .note("Position " + std::to_string(position) + " doesn't exist in this list.")
         .emit();
        return;
    }

    Node* temp = current->next;
    current->next = temp->next;
    T.at(__LINE__, __func__)
     .vars({{"position", position}, {"value", temp->data}})
     .listState(trace::listJson(head, "singly", current, {{temp, "orphan", -1}}))
     .unlink(trace::nodeId(current), trace::nodeId(temp))
     .note("Bypassing the node at position " + std::to_string(position) + ": it's left orphaned.")
     .emit();

    delete temp;
    T.at(__LINE__, __func__)
     .listState(trace::listJson(head, "singly", current))
     .note("Orphan freed.")
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

enum Mode { HEAD, TAIL, VALUE, POS };

static void run(const char* label, Mode mode, int arg, const char* intro) {
    trace::resetNodeIds();
    const int values[] = {10, 20, 30, 40, 50};
    Node* head = buildList(values, 5);

    T.input(label);
    T.at(__LINE__, "main")
     .listState(trace::listJson<Node>(head, "singly", nullptr))
     .note(intro)
     .emit();

    if (mode == HEAD) deleteAtHead(head);
    else if (mode == TAIL) deleteAtTail(head);
    else if (mode == VALUE) deleteByValue(head, arg);
    else deleteAtPosition(head, arg);

    freeList(head);
}

int main() {
    T.view("linkedlist").complexity("O(1) at the head, O(n) elsewhere", "O(1)");

    run("at the head (O(1))", HEAD, 0, "List of 5 nodes. Deleting the head.");
    run("at the tail (O(n))", TAIL, 0, "List of 5 nodes. Deleting the last node: the second-to-last is needed.");
    run("by value (30)", VALUE, 30, "Deleting the node that holds 30, as in the book.");
    run("by absent value (99)", VALUE, 99, "Deleting 99, which isn't in the list.");
    run("by position (2)", POS, 2, "Deleting the node at position 2.");

    T.dump("traces/list_delete.js");
    return 0;
}
