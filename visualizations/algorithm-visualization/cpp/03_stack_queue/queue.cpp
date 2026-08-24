// Circular Queue: enqueue / dequeue / peek
//
// FIFO on a static array. front and rear advance with the modulo, so when
// they reach the end of the array they wrap back to the start instead of
// wasting the cells already freed. count tells a full queue apart from an
// empty one, which would otherwise leave front and rear in the same
// configuration.
#include <cstdio>
#include <string>
#include "../tracer/tracer.hpp"

static Tracer T("queue", "Circular Queue: enqueue / dequeue / peek", "Stack and Queue", __FILE__);

static const int CAPACITY = 5;
static int data[CAPACITY];
static int front = 0;
static int rear = -1;
static int count = 0;

bool enqueue(int value) {
    if (count == CAPACITY) {
        T.at(__LINE__, __func__)
         .vars({{"front", front}, {"rear", rear}, {"count", count}, {"value", value}})
         .error("overflow")
         .note("Queue full: count has reached capacity = " + std::to_string(CAPACITY) +
               ". Without count there would be no way to tell this apart from an empty queue.")
         .emit();
        return false;
    }

    int oldRear = rear;
    rear = (rear + 1) % CAPACITY;
    data[rear] = value;
    count++;
    T.at(__LINE__, __func__)
     .vars({{"front", front}, {"rear", rear}, {"count", count}, {"value", value}})
     .enqueue(value)
     .counters({{"elements", count}})
     .note(oldRear == CAPACITY - 1
             ? "Wrap-around: rear = (" + std::to_string(oldRear) + " + 1) % " + std::to_string(CAPACITY) +
               " = 0. Instead of running out of space, the queue restarts from cell 0, which had been freed."
             : "Enqueue of " + std::to_string(value) + ": rear = (" + std::to_string(oldRear) + " + 1) % " +
               std::to_string(CAPACITY) + " = " + std::to_string(rear) + ".")
     .emit();
    return true;
}

int dequeue() {
    if (count == 0) {
        T.at(__LINE__, __func__)
         .vars({{"front", front}, {"rear", rear}, {"count", count}})
         .error("underflow")
         .note("Queue empty: count is 0, there is nothing to dequeue.")
         .emit();
        return -1;
    }

    int value = data[front];
    int oldFront = front;
    front = (front + 1) % CAPACITY;
    count--;
    T.at(__LINE__, __func__)
     .vars({{"front", front}, {"rear", rear}, {"count", count}, {"value", value}})
     .dequeue()
     .counters({{"elements", count}})
     .note(oldFront == CAPACITY - 1
             ? std::to_string(value) + " comes out and front wraps around too: (" + std::to_string(oldFront) +
               " + 1) % " + std::to_string(CAPACITY) + " = 0."
             : std::to_string(value) + " comes out, the first one in: front advances to " +
               std::to_string(front) + ". The freed cell will be reused on the next wrap-around.")
     .emit();
    return value;
}

int peek() {
    if (count == 0) {
        T.at(__LINE__, __func__)
         .vars({{"front", front}, {"count", count}})
         .error("underflow")
         .note("Peek on an empty queue: nothing to read.")
         .emit();
        return -1;
    }

    T.at(__LINE__, __func__)
     .vars({{"front", front}, {"rear", rear}, {"count", count}, {"value", data[front]}})
     .select(front)
     .counters({{"elements", count}})
     .note("Peek: reading " + std::to_string(data[front]) + " at the head without dequeuing it.")
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
     .counters({{"elements", 0}})
     .note(intro)
     .emit();

    for (int i = 0; i < n; i++) {
        if (ops[i].op == ENQ) enqueue(ops[i].value);
        else if (ops[i].op == DEQ) dequeue();
        else peek();
    }
}

int main() {
    T.view("queue").complexity("O(1) per operation", "O(n)");

    const Step basic[] = {{ENQ, 10}, {ENQ, 20}, {ENQ, 30}, {PEEK, 0}, {DEQ, 0}, {DEQ, 0}};
    run("basic enqueue and dequeue", basic, 6,
        "Empty queue: front = 0, rear = -1, count = 0.");

    // Built specifically to make the wrap-around actually happen and be visible.
    const Step wrap[] = {{ENQ, 10}, {ENQ, 20}, {ENQ, 30}, {DEQ, 0}, {DEQ, 0},
                         {ENQ, 40}, {ENQ, 50}, {ENQ, 60}, {ENQ, 70}, {DEQ, 0}, {DEQ, 0}};
    run("wrap-around (rear and front wrap)", wrap, 11,
        "Three enqueues, two dequeues, then more enqueues: rear reaches the end and restarts from 0.");

    const Step full[] = {{ENQ, 1}, {ENQ, 2}, {ENQ, 3}, {ENQ, 4}, {ENQ, 5}, {ENQ, 6}};
    run("full queue (overflow)", full, 6,
        "Six enqueues on capacity 5: the last one has to fail.");

    const Step empty[] = {{ENQ, 7}, {DEQ, 0}, {DEQ, 0}, {PEEK, 0}};
    run("empty queue (underflow)", empty, 4,
        "One enqueue and two dequeues: the second finds the queue empty.");

    T.dump("traces/queue.js");
    return 0;
}
