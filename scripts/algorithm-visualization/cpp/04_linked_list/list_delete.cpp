// Linked List: Delete (testa / coda / valore / posizione)
//
// Cancellare significa far scavalcare il nodo dal suo predecessore e poi
// liberarlo. Fra le due cose il nodo esiste ancora ma non è più raggiungibile:
// è un orfano, e se ci si dimenticasse della delete sarebbe un memory leak.
//
// Il libro implementa solo la cancellazione per valore: quella per posizione è
// un'aggiunta.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("list_delete", "Delete (testa / coda / valore / posizione)", "Linked List", __FILE__);

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
         .note("Lista vuota: non c'è nessuna testa da cancellare.")
         .emit();
        return;
    }

    Node* temp = head;
    head = head->next;
    T.at(__LINE__, __func__)
     .vars({{"valore", temp->data}})
     .listState(trace::listJson<Node>(head, "singly", nullptr, {{temp, "orphan", 0}}))
     .unlink("head", trace::nodeId(temp))
     .note("head passa al secondo nodo. Il vecchio primo nodo esiste ancora in memoria ma non è più "
           "raggiungibile: è un orfano.")
     .emit();

    delete temp;
    T.at(__LINE__, __func__)
     .listState(trace::listJson<Node>(head, "singly", nullptr))
     .note("delete libera la memoria dell'orfano. Cancellazione in testa: O(1).")
     .emit();
}

void deleteAtTail(Node*& head) {
    if (head == nullptr) {
        T.at(__LINE__, __func__)
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .error("underflow")
         .note("Lista vuota: niente da cancellare.")
         .emit();
        return;
    }
    if (head->next == nullptr) {
        Node* temp = head;
        head = nullptr;
        delete temp;
        T.at(__LINE__, __func__)
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .note("Unico nodo: la lista diventa vuota.")
         .emit();
        return;
    }

    Node* current = head;
    while (current->next->next != nullptr) {
        current = current->next;
        T.at(__LINE__, __func__)
         .vars({{"current->data", current->data}})
         .listState(trace::listJson(head, "singly", current))
         .note("Cerco il penultimo nodo: in una lista semplice non si può tornare indietro dalla coda, "
               "e questo è esattamente il problema che la lista doppiamente concatenata risolve.")
         .emit();
    }

    Node* temp = current->next;
    current->next = nullptr;
    T.at(__LINE__, __func__)
     .vars({{"valore", temp->data}})
     .listState(trace::listJson(head, "singly", current, {{temp, "orphan", -1}}))
     .unlink(trace::nodeId(current), trace::nodeId(temp))
     .note("Il penultimo diventa la nuova coda (next = NULL) e l'ultimo resta orfano.")
     .emit();

    delete temp;
    T.at(__LINE__, __func__)
     .listState(trace::listJson<Node>(head, "singly", nullptr))
     .note("Orfano liberato. Il costo è tutto nella scansione: O(n).")
     .emit();
}

void deleteByValue(Node*& head, int value) {
    if (head == nullptr) {
        T.at(__LINE__, __func__)
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .error("underflow")
         .note("Lista vuota.")
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
         .note("Guardo il valore del nodo *successivo*: per scavalcarlo serve tenere in mano il suo "
               "predecessore.")
         .emit();
        current = current->next;
    }

    if (current->next == nullptr) {
        T.at(__LINE__, __func__)
         .vars({{"value", value}})
         .listState(trace::listJson(head, "singly", current))
         .error("not_found")
         .note("Arrivato in fondo senza trovare " + std::to_string(value) + ": non c'è niente da cancellare.")
         .emit();
        return;
    }

    Node* temp = current->next;
    current->next = temp->next;
    T.at(__LINE__, __func__)
     .vars({{"value", value}})
     .listState(trace::listJson(head, "singly", current, {{temp, "orphan", -1}}))
     .unlink(trace::nodeId(current), trace::nodeId(temp))
     .note("Puntatore di scavalco: current->next salta il nodo da cancellare e va direttamente al "
           "successivo. Il nodo scavalcato è ora orfano.")
     .emit();

    delete temp;
    T.at(__LINE__, __func__)
     .listState(trace::listJson(head, "singly", current))
     .note("delete libera l'orfano: la lista è di nuovo intera, con un nodo in meno.")
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
         .note("Avanzo fino al predecessore della posizione " + std::to_string(position) + ".")
         .emit();
    }

    if (current == nullptr || current->next == nullptr) {
        T.at(__LINE__, __func__)
         .vars({{"position", position}})
         .listState(trace::listJson<Node>(head, "singly", nullptr))
         .error("out_of_range")
         .note("La posizione " + std::to_string(position) + " non esiste in questa lista.")
         .emit();
        return;
    }

    Node* temp = current->next;
    current->next = temp->next;
    T.at(__LINE__, __func__)
     .vars({{"position", position}, {"valore", temp->data}})
     .listState(trace::listJson(head, "singly", current, {{temp, "orphan", -1}}))
     .unlink(trace::nodeId(current), trace::nodeId(temp))
     .note("Scavalco il nodo in posizione " + std::to_string(position) + ": resta orfano.")
     .emit();

    delete temp;
    T.at(__LINE__, __func__)
     .listState(trace::listJson(head, "singly", current))
     .note("Orfano liberato.")
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
    T.view("linkedlist").complexity("O(1) in testa, O(n) altrove", "O(1)");

    run("in testa (O(1))", HEAD, 0, "Lista di 5 nodi. Cancello la testa.");
    run("in coda (O(n))", TAIL, 0, "Lista di 5 nodi. Cancello l'ultimo nodo: serve il penultimo.");
    run("per valore (30)", VALUE, 30, "Cancello il nodo che contiene 30, come nel libro.");
    run("per valore assente (99)", VALUE, 99, "Cancello 99, che nella lista non c'è.");
    run("per posizione (2)", POS, 2, "Cancello il nodo in posizione 2.");

    T.dump("traces/list_delete.js");
    return 0;
}
