// Linked List: Accessing
//
// Una lista non ha aritmetica dell'indirizzo: per arrivare all'elemento i
// bisogna partire dalla testa e seguire i puntatori i volte. È la differenza
// che conta rispetto all'array: accesso O(n) invece di O(1).
//
// Il libro non implementa questa operazione: getAt() è un'aggiunta.
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
     .counters({{"salti", hops}})
     .note("Parto dalla testa. Non posso saltare: l'unico modo di avanzare è seguire next.")
     .emit();

    while (current != nullptr && i < index) {
        current = current->next;
        i++;
        T.at(__LINE__, __func__)
         .vars({{"index", index}, {"i", i}})
         .listState(trace::listJson(head, "singly", current))
         .counters({{"salti", ++hops}})
         .note("Un salto: i = " + std::to_string(i) + ". Servono esattamente " + std::to_string(index) +
               " salti per arrivare alla posizione " + std::to_string(index) + ".")
         .emit();
    }

    if (current == nullptr) {
        T.at(__LINE__, __func__)
         .vars({{"index", index}, {"i", i}})
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .error("out_of_range")
         .counters({{"salti", hops}})
         .note("current è NULL: la lista è finita prima di arrivare alla posizione " +
               std::to_string(index) + ". Ritorno -1.")
         .emit();
        return -1;
    }

    T.at(__LINE__, __func__)
     .vars({{"index", index}, {"valore", current->data}})
     .listState(trace::listJson(head, "singly", current))
     .counters({{"salti", hops}})
     .note("Arrivato: l'elemento in posizione " + std::to_string(index) + " vale " +
           std::to_string(current->data) + ", raggiunto con " + std::to_string(hops) + " salti.")
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
        {"prima posizione (i = 0)", 0},
        {"posizione centrale (i = 2)", 2},
        {"ultima posizione (i = 4)", 4},
        {"posizione inesistente (i = 7)", 7},
    };

    for (const Case& c : cases) {
        Node* head = buildList(values, 5);
        hops = 0;
        T.input(c.label);
        trace::resetNodeIds();
        T.at(__LINE__, "getAt")
         .vars({{"index", c.index}})
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .counters({{"salti", 0}})
         .note("Lista di 5 nodi. Cerco la posizione " + std::to_string(c.index) + ".")
         .emit();

        getAt(head, c.index);
        freeList(head);
    }

    T.dump("traces/list_access.js");
    return 0;
}
