// Linked List: Search
//
// Confronto nodo per nodo partendo dalla testa. Anche se la lista fosse
// ordinata non si potrebbe fare una ricerca binaria: non c'è modo di saltare
// al centro senza percorrere i puntatori.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("list_search", "Search", "Linked List", __FILE__);
static int comparisons = 0;

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

int search(Node* head, int key) {
    Node* current = head;
    int position = 0;

    while (current != nullptr) {
        T.at(__LINE__, __func__)
         .vars({{"position", position}, {"key", key}, {"current->data", current->data}})
         .listState(trace::listJson(head, "singly", current))
         .counters({{"confronti", ++comparisons}})
         .note("Confronto il nodo in posizione " + std::to_string(position) + " (" +
               std::to_string(current->data) + ") con la chiave " + std::to_string(key) + ".")
         .emit();

        if (current->data == key) {
            T.at(__LINE__, __func__)
             .vars({{"position", position}, {"key", key}})
             .listState(trace::listJson(head, "singly", current))
             .found(position)
             .counters({{"confronti", comparisons}})
             .note("Trovata in posizione " + std::to_string(position) + " dopo " +
                   std::to_string(comparisons) + " confronti.")
             .emit();
            return position;
        }

        current = current->next;
        position++;
    }

    T.at(__LINE__, __func__)
     .vars({{"key", key}, {"position", position}})
     .listState(trace::listJson<Node>(head, "singly", nullptr))
     .counters({{"confronti", comparisons}})
     .note("current è NULL: la chiave " + std::to_string(key) + " non è nella lista. Ritorno -1 dopo " +
           std::to_string(comparisons) + " confronti, cioè tutta la lista.")
     .emit();
    return -1;
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
    struct Case { const char* label; int key; };
    const Case cases[] = {
        {"chiave in testa (10)", 10},
        {"chiave in mezzo (30)", 30},
        {"chiave in coda (50)", 50},
        {"chiave assente (35)", 35},
    };

    for (const Case& c : cases) {
        trace::resetNodeIds();
        Node* head = buildList(values, 5);
        comparisons = 0;
        T.input(c.label);
        T.at(__LINE__, "search")
         .vars({{"key", c.key}})
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .counters({{"confronti", 0}})
         .note("Lista di 5 nodi, cerco il valore " + std::to_string(c.key) + ".")
         .emit();

        search(head, c.key);
        freeList(head);
    }

    T.dump("traces/list_search.js");
    return 0;
}
