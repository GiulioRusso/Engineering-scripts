// Stack: push / pop / peek
//
// LIFO su array statico. top è l'indice dell'ultimo elemento inserito e vale -1
// quando lo stack è vuoto: è quel -1 a rendere il controllo di underflow una
// sola condizione. Ogni operazione costa O(1), non c'è nessuno spostamento.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"

static Tracer T("stack", "Stack: push / pop / peek", "Stack e Queue", __FILE__);

static const int CAPACITY = 6;
static int data[CAPACITY];
static int top = -1;

bool push(int value) {
    if (top == CAPACITY - 1) {
        T.at(__LINE__, __func__)
         .vars({{"top", top}, {"capacity", CAPACITY}, {"valore", value}})
         .error("overflow")
         .note("Overflow: top è già capacity-1 = " + std::to_string(CAPACITY - 1) +
               ". Un array statico non cresce, quindi la push fallisce.")
         .emit();
        return false;
    }

    top++;
    data[top] = value;
    T.at(__LINE__, __func__)
     .vars({{"top", top}, {"valore", value}})
     .push(value)
     .counters({{"elementi", top + 1}})
     .note("Push di " + std::to_string(value) + ": top sale a " + std::to_string(top) +
           " e il valore viene scritto lì. Nessun elemento si sposta, quindi O(1).")
     .emit();
    return true;
}

int pop() {
    if (top == -1) {
        T.at(__LINE__, __func__)
         .vars({{"top", top}})
         .error("underflow")
         .note("Underflow: top vale -1, lo stack è vuoto e non c'è niente da estrarre.")
         .emit();
        return -1;
    }

    int value = data[top];
    top--;
    T.at(__LINE__, __func__)
     .vars({{"top", top}, {"valore", value}})
     .pop()
     .counters({{"elementi", top + 1}})
     .note("Pop: esce " + std::to_string(value) + " ed è l'ultimo entrato — è questo il LIFO. "
           "Il valore resta scritto in memoria, ma con top = " + std::to_string(top) +
           " non fa più parte dello stack.")
     .emit();
    return value;
}

int peek() {
    if (top == -1) {
        T.at(__LINE__, __func__)
         .vars({{"top", top}})
         .error("underflow")
         .note("Peek su stack vuoto: niente da leggere.")
         .emit();
        return -1;
    }

    T.at(__LINE__, __func__)
     .vars({{"top", top}, {"valore", data[top]}})
     .select(top)
     .counters({{"elementi", top + 1}})
     .note("Peek: leggo " + std::to_string(data[top]) + " senza toccare top. Lo stack non cambia.")
     .emit();
    return data[top];
}

// Ogni input è una sequenza di operazioni scritta a mano, scelta per far
// vedere anche i casi limite.
enum Op { PUSH, POP, PEEK };
struct Step { Op op; int value; };

static void run(const char* label, const Step* ops, int count, const char* intro) {
    for (int i = 0; i < CAPACITY; i++) data[i] = 0;
    top = -1;

    T.input(label);
    T.clearWatches().watchArray("data", data, CAPACITY);
    T.at(__LINE__, "main")
     .vars({{"top", top}, {"capacity", CAPACITY}})
     .counters({{"elementi", 0}})
     .note(intro)
     .emit();

    for (int i = 0; i < count; i++) {
        if (ops[i].op == PUSH) push(ops[i].value);
        else if (ops[i].op == POP) pop();
        else peek();
    }
}

int main() {
    T.view("stack").complexity("O(1) per operazione", "O(n)");

    const Step book[] = {{PUSH, 10}, {PUSH, 20}, {PUSH, 30}, {POP, 0}, {PEEK, 0}};
    run("sequenza del libro (push 10, 20, 30 → pop)", book, 5,
        "Stack vuoto: top = -1. Seguono push 10, push 20, push 30, poi una pop.");

    const Step over[] = {{PUSH, 1}, {PUSH, 2}, {PUSH, 3}, {PUSH, 4}, {PUSH, 5}, {PUSH, 6}, {PUSH, 7}};
    run("overflow (7 push su capacity 6)", over, 7,
        "Sette push su una capacity di sei: l'ultima deve fallire.");

    const Step under[] = {{PUSH, 42}, {POP, 0}, {POP, 0}, {PEEK, 0}};
    run("underflow (pop su stack vuoto)", under, 4,
        "Una push, poi due pop: la seconda trova lo stack vuoto.");

    const Step mixed[] = {{PUSH, 5}, {PUSH, 9}, {POP, 0}, {PUSH, 7}, {PUSH, 3}, {POP, 0}, {POP, 0}, {POP, 0}};
    run("sequenza mista", mixed, 8,
        "Push e pop alternate: il riuso delle celle sopra top si vede bene.");

    T.dump("traces/stack.js");
    return 0;
}
