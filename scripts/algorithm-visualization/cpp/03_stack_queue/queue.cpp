// Queue circolare: enqueue / dequeue / peek
//
// FIFO su array statico. front e rear avanzano con il modulo, così quando
// arrivano in fondo all'array ricominciano dall'inizio invece di sprecare le
// celle già liberate. count distingue coda piena da coda vuota, che altrimenti
// darebbero la stessa configurazione di front e rear.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"

static Tracer T("queue", "Queue circolare: enqueue / dequeue / peek", "Stack e Queue", __FILE__);

static const int CAPACITY = 5;
static int data[CAPACITY];
static int front = 0;
static int rear = -1;
static int count = 0;

bool enqueue(int value) {
    if (count == CAPACITY) {
        T.at(__LINE__, __func__)
         .vars({{"front", front}, {"rear", rear}, {"count", count}, {"valore", value}})
         .error("overflow")
         .note("Coda piena: count è arrivato a capacity = " + std::to_string(CAPACITY) +
               ". Senza count non si potrebbe distinguere questo caso dalla coda vuota.")
         .emit();
        return false;
    }

    int oldRear = rear;
    rear = (rear + 1) % CAPACITY;
    data[rear] = value;
    count++;
    T.at(__LINE__, __func__)
     .vars({{"front", front}, {"rear", rear}, {"count", count}, {"valore", value}})
     .enqueue(value)
     .counters({{"elementi", count}})
     .note(oldRear == CAPACITY - 1
             ? "Avvolgimento: rear = (" + std::to_string(oldRear) + " + 1) % " + std::to_string(CAPACITY) +
               " = 0. Invece di finire lo spazio, la coda riparte dalla cella 0 che era stata liberata."
             : "Enqueue di " + std::to_string(value) + ": rear = (" + std::to_string(oldRear) + " + 1) % " +
               std::to_string(CAPACITY) + " = " + std::to_string(rear) + ".")
     .emit();
    return true;
}

int dequeue() {
    if (count == 0) {
        T.at(__LINE__, __func__)
         .vars({{"front", front}, {"rear", rear}, {"count", count}})
         .error("underflow")
         .note("Coda vuota: count vale 0, non c'è niente da estrarre.")
         .emit();
        return -1;
    }

    int value = data[front];
    int oldFront = front;
    front = (front + 1) % CAPACITY;
    count--;
    T.at(__LINE__, __func__)
     .vars({{"front", front}, {"rear", rear}, {"count", count}, {"valore", value}})
     .dequeue()
     .counters({{"elementi", count}})
     .note(oldFront == CAPACITY - 1
             ? "Esce " + std::to_string(value) + " e anche front si avvolge: (" + std::to_string(oldFront) +
               " + 1) % " + std::to_string(CAPACITY) + " = 0."
             : "Esce " + std::to_string(value) + ", il primo entrato: front avanza a " +
               std::to_string(front) + ". La cella liberata sarà riusata dal prossimo avvolgimento.")
     .emit();
    return value;
}

int peek() {
    if (count == 0) {
        T.at(__LINE__, __func__)
         .vars({{"front", front}, {"count", count}})
         .error("underflow")
         .note("Peek su coda vuota: niente da leggere.")
         .emit();
        return -1;
    }

    T.at(__LINE__, __func__)
     .vars({{"front", front}, {"rear", rear}, {"count", count}, {"valore", data[front]}})
     .select(front)
     .counters({{"elementi", count}})
     .note("Peek: leggo " + std::to_string(data[front]) + " in testa senza estrarlo.")
     .emit();
    return data[front];
}

enum Op { ENQ, DEQ, PEEK };
struct Step { Op op; int value; };

static void run(const char* label, const Step* ops, int n, const char* intro) {
    for (int i = 0; i < CAPACITY; i++) data[i] = 0;
    front = 0;
    rear = -1;
    count = 0;

    T.input(label);
    T.clearWatches().watchArray("data", data, CAPACITY);
    T.at(__LINE__, "main")
     .vars({{"front", front}, {"rear", rear}, {"count", count}})
     .counters({{"elementi", 0}})
     .note(intro)
     .emit();

    for (int i = 0; i < n; i++) {
        if (ops[i].op == ENQ) enqueue(ops[i].value);
        else if (ops[i].op == DEQ) dequeue();
        else peek();
    }
}

int main() {
    T.view("queue").complexity("O(1) per operazione", "O(n)");

    const Step basic[] = {{ENQ, 10}, {ENQ, 20}, {ENQ, 30}, {PEEK, 0}, {DEQ, 0}, {DEQ, 0}};
    run("enqueue e dequeue di base", basic, 6,
        "Coda vuota: front = 0, rear = -1, count = 0.");

    // Costruita apposta perché l'avvolgimento avvenga davvero e si veda.
    const Step wrap[] = {{ENQ, 10}, {ENQ, 20}, {ENQ, 30}, {DEQ, 0}, {DEQ, 0},
                         {ENQ, 40}, {ENQ, 50}, {ENQ, 60}, {ENQ, 70}, {DEQ, 0}, {DEQ, 0}};
    run("wrap-around (rear e front si avvolgono)", wrap, 11,
        "Tre enqueue, due dequeue, poi altre enqueue: rear arriva in fondo e riparte da 0.");

    const Step full[] = {{ENQ, 1}, {ENQ, 2}, {ENQ, 3}, {ENQ, 4}, {ENQ, 5}, {ENQ, 6}};
    run("coda piena (overflow)", full, 6,
        "Sei enqueue su capacity 5: l'ultima deve fallire.");

    const Step empty[] = {{ENQ, 7}, {DEQ, 0}, {DEQ, 0}, {PEEK, 0}};
    run("coda vuota (underflow)", empty, 4,
        "Una enqueue e due dequeue: la seconda trova la coda vuota.");

    T.dump("traces/queue.js");
    return 0;
}
