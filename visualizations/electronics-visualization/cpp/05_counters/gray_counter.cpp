// Gray Code — One Bit Changes at a Time
//
// Ordinary binary can change several bits in the same step (011 -> 100
// changes three): in a real circuit those bits don't switch at the exact
// same instant, and the transient state read mid-switch is garbage. Gray
// code guarantees exactly one bit changes per step, eliminating that problem
// by construction: g = n XOR (n >> 1).
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"

static Tracer T("gray_counter", "Gray Code", "Counters", __FILE__);
static const int NBITS = 4;

static std::vector<int> bits(int n) {
    std::vector<int> b(NBITS);
    for (int i = 0; i < NBITS; i++) b[i] = (n >> i) & 1;
    return b;
}

static int flipCount(const std::vector<int>& a, const std::vector<int>& b) {
    int c = 0;
    for (int i = 0; i < NBITS; i++) if (a[i] != b[i]) c++;
    return c;
}

int main() {
    T.view("array").complexity("O(1) per step", "XOR + shift");
    T.panel("table", "Bits that change: binary vs Gray");

    std::vector<int> reg = bits(0);
    T.clearWatches();
    T.watchVector("gray", reg);

    T.input("0 .. 15 in Gray code");
    std::vector<int> prevBin = bits(0), prevGray = bits(0);
    for (int n = 0; n <= 15; n++) {
        int g = n ^ (n >> 1);
        reg = bits(g);
        std::vector<int> binNow = bits(n);
        int binFlips = n > 0 ? flipCount(prevBin, binNow) : 0;
        int grayFlips = n > 0 ? flipCount(prevGray, reg) : 0;
        T.at(__LINE__, __func__)
         .vars({{"n", n}, {"gray", g}, {"bits_changed_binary", binFlips}, {"bits_changed_gray", grayFlips}})
         .table("n=" + std::to_string(n), {{"binary", binFlips}, {"gray", grayFlips}})
         .note("g = " + std::to_string(n) + " XOR (" + std::to_string(n) + " >> 1) = " +
               std::to_string(g) + ". Ordinary binary would have changed " + std::to_string(binFlips) +
               " bits here, Gray changes " + std::to_string(grayFlips) + ".")
         .emit();
        prevBin = binNow;
        prevGray = reg;
    }

    T.dump("traces/gray_counter.js");
}
