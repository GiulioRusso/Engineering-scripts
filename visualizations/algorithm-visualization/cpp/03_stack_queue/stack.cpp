// Stack: push / pop / peek
//
// LIFO on a static array. top is the index of the last inserted element and
// is -1 when the stack is empty: that -1 is what makes the underflow check a
// single condition. Every operation costs O(1), nothing ever shifts.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"

static Tracer T("stack", "Stack: push / pop / peek", "Stack and Queue", __FILE__);

static const int CAPACITY = 6;
static int data[CAPACITY];
static int top = -1;

bool push(int value) {
    if (top == CAPACITY - 1) {
        T.at(__LINE__, __func__)
         .vars({{"top", top}, {"capacity", CAPACITY}, {"value", value}})
         .error("overflow")
         .note("Overflow: top is already capacity-1 = " + std::to_string(CAPACITY - 1) +
               ". A static array doesn't grow, so the push fails.")
         .emit();
        return false;
    }

    top++;
    data[top] = value;
    T.at(__LINE__, __func__)
     .vars({{"top", top}, {"value", value}})
     .push(value)
     .counters({{"elements", top + 1}})
     .note("Push of " + std::to_string(value) + ": top rises to " + std::to_string(top) +
           " and the value is written there. No element moves, so O(1).")
     .emit();
    return true;
}

int pop() {
    if (top == -1) {
        T.at(__LINE__, __func__)
         .vars({{"top", top}})
         .error("underflow")
         .note("Underflow: top is -1, the stack is empty and there is nothing to pop.")
         .emit();
        return -1;
    }

    int value = data[top];
    top--;
    T.at(__LINE__, __func__)
     .vars({{"top", top}, {"value", value}})
     .pop()
     .counters({{"elements", top + 1}})
     .note("Pop: " + std::to_string(value) + " comes out, and it's the last one that went in — that's "
           "the LIFO. The value stays written in memory, but with top = " + std::to_string(top) +
           " it's no longer part of the stack.")
     .emit();
    return value;
}

int peek() {
    if (top == -1) {
        T.at(__LINE__, __func__)
         .vars({{"top", top}})
         .error("underflow")
         .note("Peek on an empty stack: nothing to read.")
         .emit();
        return -1;
    }

    T.at(__LINE__, __func__)
     .vars({{"top", top}, {"value", data[top]}})
     .select(top)
     .counters({{"elements", top + 1}})
     .note("Peek: reading " + std::to_string(data[top]) + " without touching top. The stack doesn't change.")
     .emit();
    return data[top];
}

// Each input is a hand-written sequence of operations, chosen to also show
// the edge cases.
enum Op { PUSH, POP, PEEK };
struct Step { Op op; int value; };

static void run(const char* label, const Step* ops, int count, const char* intro) {
    for (int i = 0; i < CAPACITY; i++) data[i] = 0;
    top = -1;

    T.input(label);
    T.clearWatches().watchArray("data", data, CAPACITY);
    T.at(__LINE__, "main")
     .vars({{"top", top}, {"capacity", CAPACITY}})
     .counters({{"elements", 0}})
     .note(intro)
     .emit();

    for (int i = 0; i < count; i++) {
        if (ops[i].op == PUSH) push(ops[i].value);
        else if (ops[i].op == POP) pop();
        else peek();
    }
}

int main() {
    T.view("stack").complexity("O(1) per operation", "O(n)");

    const Step book[] = {{PUSH, 10}, {PUSH, 20}, {PUSH, 30}, {POP, 0}, {PEEK, 0}};
    run("textbook sequence (push 10, 20, 30 → pop)", book, 5,
        "Empty stack: top = -1. Followed by push 10, push 20, push 30, then a pop.");

    const Step over[] = {{PUSH, 1}, {PUSH, 2}, {PUSH, 3}, {PUSH, 4}, {PUSH, 5}, {PUSH, 6}, {PUSH, 7}};
    run("overflow (7 pushes on capacity 6)", over, 7,
        "Seven pushes on a capacity of six: the last one has to fail.");

    const Step under[] = {{PUSH, 42}, {POP, 0}, {POP, 0}, {PEEK, 0}};
    run("underflow (pop on an empty stack)", under, 4,
        "One push, then two pops: the second finds the stack empty.");

    const Step mixed[] = {{PUSH, 5}, {PUSH, 9}, {POP, 0}, {PUSH, 7}, {PUSH, 3}, {POP, 0}, {POP, 0}, {POP, 0}};
    run("mixed sequence", mixed, 8,
        "Alternating pushes and pops: the reuse of cells above top shows up clearly.");

    T.dump("traces/stack.js");
    return 0;
}
