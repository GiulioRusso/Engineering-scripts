// Singly / Doubly / Circular
//
// Non un algoritmo ma un confronto: le stesse operazioni sulle tre varianti,
// per vedere che cosa cambia nei puntatori e che cosa questo rende possibile.
// Il libro le definisce senza implementarle: qui sono implementate.
//
//   singly   : un solo next, la coda punta a NULL, non si torna indietro
//   doubly   : anche prev, si attraversa nei due sensi, un puntatore in più per nodo
//   circular : la coda punta alla testa, non esiste un NULL su cui fermarsi
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"
#include "../tracer/views.hpp"

static Tracer T("list_types", "Singly / Doubly / Circular", "Linked List", __FILE__);

struct Node {
    int data;
    Node* next;
    Node* prev;   // usato solo dalla lista doppiamente concatenata
};

Node* createNode(int value) {
    Node* newNode = new Node;
    newNode->data = value;
    newNode->next = nullptr;
    newNode->prev = nullptr;
    return newNode;
}

// Nella lista semplice la coda resta con next = NULL.
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

// Nella lista doppia ogni nodo ricorda anche il precedente.
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

// Nella lista circolare la coda punta alla testa: nessun NULL da nessuna parte.
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

// Inserimento in testa sulle tre varianti: quello che cambia è quanti puntatori
// vanno sistemati, non il costo, che resta O(1) ovunque.
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
     .vars({{"valore", value}})
     .listState(trace::listJson(singly, "singly", singly))
     .listState("doubly", trace::listJson(doubly, "doubly", doubly))
     .listState("circular", trace::listJson(circular, "circular", circular))
     .link("head", trace::nodeId(a))
     .note("Inserimento in testa. Semplice: un aggancio. Doppia: due, perché anche il vecchio primo "
           "nodo deve ricordare il nuovo (prev). Circolare: due, ma uno richiede di trovare la coda, "
           "e trovarla costa O(n) se non la si tiene da parte.")
     .emit();
}

int main() {
    T.view("linkedlist").complexity("O(1) inserimento in testa", "O(n)");
    T.panel("linkedlist", "Doppiamente concatenata", "doubly");
    T.panel("linkedlist", "Circolare", "circular");

    const int values[] = {10, 20, 30, 40};

    trace::resetNodeIds();
    singly = buildSingly(values, 4);
    doubly = buildDoubly(values, 4);
    circular = buildCircular(values, 4);

    T.input("struttura dei puntatori a confronto");
    snapshot(__LINE__, "main",
             "Le stesse quattro celle, tre strutture diverse. In alto la lista semplice: l'ultimo next "
             "è NULL. Nel primo pannello la doppia: sotto la catena si vedono i prev. Nel secondo la "
             "circolare: la coda torna alla testa e di NULL non ce n'è.");
    snapshot(__LINE__, "main",
             "Conseguenza pratica: nella semplice, per cancellare la coda bisogna scandire fino al "
             "penultimo. Nella doppia si arriva alla coda e si legge il suo prev. Nella circolare non "
             "esiste una condizione di fine: il ciclo va fermato contando, o confrontando con head.",
             singly);
    insertHeadAll(5);

    T.input("attraversamento");
    trace::resetNodeIds();
    singly = buildSingly(values, 4);
    doubly = buildDoubly(values, 4);
    circular = buildCircular(values, 4);

    Node* p = singly;
    int steps = 0;
    while (p != nullptr) {
        snapshot(__LINE__, "main",
                 "Semplice: avanzo finché non trovo NULL. Passo " + std::to_string(++steps) +
                 ". Nella circolare la stessa condizione non terminerebbe mai: il cursore tornerebbe "
                 "sulla testa e ricomincerebbe.", p);
        p = p->next;
    }
    snapshot(__LINE__, "main",
             "p è NULL: l'attraversamento della lista semplice finisce qui. Per la circolare la "
             "condizione giusta è `do { ... } while (p != head)`.");

    T.dump("traces/list_types.js");
    return 0;
}
