// Linked List: Insert (testa / coda / posizione)
//
// L'inserimento è O(1) se si ha già in mano il nodo precedente: si allocano e
// si riagganciano due puntatori, non si sposta nessun dato. Il costo, quando
// c'è, sta tutto nel raggiungere quel nodo precedente.
//
// Il libro implementa solo testa e coda: l'inserimento in posizione è
// un'aggiunta.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("list_insert", "Insert (testa / coda / posizione)", "Linked List", __FILE__);

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
     .vars({{"valore", value}})
     .listState(trace::listJson<Node>(head, "singly", nullptr, {{newNode, "new", 0}}))
     .note("Nodo allocato fuori dalla catena: il suo next è NULL e nessuno lo raggiunge ancora.")
     .emit();

    newNode->next = head;
    T.at(__LINE__, __func__)
     .vars({{"valore", value}})
     .listState(trace::listJson<Node>(head, "singly", nullptr, {{newNode, "new", 0}}))
     .link(trace::nodeId(newNode), trace::nodeId(newNode->next))
     .note("Primo aggancio: newNode->next punta alla vecchia testa. La lista è ancora raggiungibile da head.")
     .emit();

    head = newNode;
    T.at(__LINE__, __func__)
     .vars({{"valore", value}})
     .listState(trace::listJson(head, "singly", newNode))
     .link("head", trace::nodeId(newNode))
     .note("Secondo aggancio: head passa al nodo nuovo. Inserimento in testa: O(1), nessuna scansione.")
     .emit();
}

void insertAtTail(Node*& head, int value) {
    Node* newNode = createNode(value);
    if (head == nullptr) {
        head = newNode;
        T.at(__LINE__, __func__)
         .vars({{"valore", value}})
         .listState(trace::listJson(head, "singly", newNode))
         .note("Lista vuota: il nodo nuovo diventa direttamente la testa.")
         .emit();
        return;
    }

    Node* current = head;
    while (current->next != nullptr) {
        current = current->next;
        T.at(__LINE__, __func__)
         .vars({{"valore", value}, {"current->data", current->data}})
         .listState(trace::listJson(head, "singly", current, {{newNode, "new", -1}}))
         .note("Cerco la coda seguendo i next. È questa scansione a rendere l'inserimento in coda O(n).")
         .emit();
    }

    current->next = newNode;
    T.at(__LINE__, __func__)
     .vars({{"valore", value}})
     .listState(trace::listJson(head, "singly", newNode))
     .link(trace::nodeId(current), trace::nodeId(newNode))
     .note("Trovata la coda (next == NULL): la aggancio al nodo nuovo, che diventa la nuova coda.")
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
         .note("Avanzo fino al nodo che precede la posizione " + std::to_string(position) + ".")
         .emit();
    }

    if (current == nullptr) {
        T.at(__LINE__, __func__)
         .vars({{"position", position}})
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .error("out_of_range")
         .note("La lista finisce prima della posizione " + std::to_string(position) +
               ": non c'è un nodo precedente a cui agganciarsi.")
         .emit();
        return;
    }

    Node* newNode = createNode(value);
    T.at(__LINE__, __func__)
     .vars({{"position", position}, {"valore", value}})
     .listState(trace::listJson(head, "singly", current, {{newNode, "new", position}}))
     .note("Nodo nuovo allocato sopra il punto di inserimento, ancora scollegato.")
     .emit();

    newNode->next = current->next;
    T.at(__LINE__, __func__)
     .vars({{"position", position}, {"valore", value}})
     .listState(trace::listJson(head, "singly", current, {{newNode, "new", position}}))
     .link(trace::nodeId(newNode), trace::nodeId(newNode->next))
     .note("PRIMO: newNode->next prende il resto della lista. Se si invertisse l'ordine dei due "
           "agganci, current->next verrebbe sovrascritto e tutta la coda della lista andrebbe persa.")
     .emit();

    current->next = newNode;
    T.at(__LINE__, __func__)
     .vars({{"position", position}, {"valore", value}})
     .listState(trace::listJson(head, "singly", newNode))
     .link(trace::nodeId(current), trace::nodeId(newNode))
     .note("SECONDO: current->next punta al nodo nuovo. Ora la catena è di nuovo intera e contiene "
           "un elemento in più.")
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
    T.view("linkedlist").complexity("O(1) in testa, O(n) in coda o in posizione", "O(1)");

    run("in testa (O(1))", HEAD, 5, 0, "Lista di 4 nodi. Inserisco 5 in testa.");
    run("in coda (O(n))", TAIL, 50, 0, "Lista di 4 nodi. Inserisco 50 in coda: prima va trovata.");
    run("in posizione 2", POS, 25, 2, "Lista di 4 nodi. Inserisco 25 in posizione 2, fra 20 e 30.");
    run("in posizione 0 (ricade sulla testa)", POS, 5, 0, "Posizione 0: il caso si riduce all'inserimento in testa.");
    run("posizione oltre la fine", POS, 99, 9, "Posizione 9 su una lista di 4 nodi: deve fallire.");

    T.dump("traces/list_insert.js");
    return 0;
}
