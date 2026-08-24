// Linked List: Accessing
//
// A list has no address arithmetic: reaching element i means starting at the
// head and following pointers i times. That's the difference that matters
// versus an array: O(n) access instead of O(1).
//
// The book doesn't implement this operation: getAt() is an addition.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("list_access", "Accessing", "Linked List", __FILE__);
static int hops = 0;

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

int getAt(Node* head, int index) {
    Node* current = head;
    int i = 0;
    T.at(__LINE__, __func__)
     .vars({{"index", index}, {"i", i}})
     .listState(trace::listJson(head, "singly", current))
     .counters({{"hops", hops}})
     .note("Starting at the head. No shortcuts: the only way forward is following next.")
     .emit();

    while (current != nullptr && i < index) {
        current = current->next;
        i++;
        T.at(__LINE__, __func__)
         .vars({{"index", index}, {"i", i}})
         .listState(trace::listJson(head, "singly", current))
         .counters({{"hops", ++hops}})
         .note("One hop: i = " + std::to_string(i) + ". It takes exactly " + std::to_string(index) +
               " hops to reach position " + std::to_string(index) + ".")
         .emit();
    }

    if (current == nullptr) {
        T.at(__LINE__, __func__)
         .vars({{"index", index}, {"i", i}})
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .error("out_of_range")
         .counters({{"hops", hops}})
         .note("current is NULL: the list ran out before reaching position " +
               std::to_string(index) + ". Returning -1.")
         .emit();
        return -1;
    }

    T.at(__LINE__, __func__)
     .vars({{"index", index}, {"value", current->data}})
     .listState(trace::listJson(head, "singly", current))
     .counters({{"hops", hops}})
     .note("Arrived: the element at position " + std::to_string(index) + " is " +
           std::to_string(current->data) + ", reached with " + std::to_string(hops) + " hops.")
     .emit();
    return current->data;
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

int main() {
    T.view("linkedlist").complexity("O(n)", "O(1)");

    const int values[] = {10, 20, 30, 40, 50};
    struct Case { const char* label; int index; };
    const Case cases[] = {
        {"first position (i = 0)", 0},
        {"middle position (i = 2)", 2},
        {"last position (i = 4)", 4},
        {"nonexistent position (i = 7)", 7},
    };

    for (const Case& c : cases) {
        Node* head = buildList(values, 5);
        hops = 0;
        T.input(c.label);
        trace::resetNodeIds();
        T.at(__LINE__, "getAt")
         .vars({{"index", c.index}})
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .counters({{"hops", 0}})
         .note("List of 5 nodes. Looking for position " + std::to_string(c.index) + ".")
         .emit();

        getAt(head, c.index);
        freeList(head);
    }

    T.dump("traces/list_access.js");
    return 0;
}
