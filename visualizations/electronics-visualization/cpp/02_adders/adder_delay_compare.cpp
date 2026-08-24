// Ripple vs Carry-Look-Ahead — Why the Circuit's Shape Matters
//
// An n-bit ripple-carry has to wait for n cascaded carries: the delay grows
// linearly with the width. A carry-look-ahead computes every carry directly
// from the inputs with a fixed level of gates: the delay stays constant.
// Same result, different circuit shape.
#include <string>
#include "../tracer/tracer.hpp"

static Tracer T("adder_delay_compare", "Ripple vs Carry-Look-Ahead", "Adders", __FILE__);

int main() {
    T.view("table").complexity("O(n)", "O(1)");
    T.input("Delay as width grows");

    const int NMAX = 8;
    for (int n = 1; n <= NMAX; n++) {
        int ripple = 2 * n;
        int cla = 4;
        auto b = T.at(__LINE__, __func__);
        b.vars({{"n", n}, {"ripple delay", ripple}, {"CLA delay", cla}});
        for (int m = 1; m <= n; m++) {
            b.table("n=" + std::to_string(m), {{"ripple", 2 * m}, {"CLA", cla}});
        }
        b.note("At " + std::to_string(n) + " bits: ripple-carry takes " + std::to_string(ripple) +
               " cascaded gate delays, carry-look-ahead takes " + std::to_string(cla) +
               " regardless of n.");
        b.emit();
    }

    T.dump("traces/adder_delay_compare.js");
}
