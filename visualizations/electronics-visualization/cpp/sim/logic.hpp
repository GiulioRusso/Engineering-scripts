// logic.hpp - the seven gates, as plain functions on 0/1 ints.
//
// Same definitions as scripts/electronics/logic_gates.ipynb. A circuit .cpp
// calls these directly; the tracer only records what they produced.
#pragma once

namespace sim {

inline int NOT(int a)         { return 1 - a; }
inline int AND(int a, int b)  { return a & b; }
inline int OR(int a, int b)   { return a | b; }
inline int NAND(int a, int b) { return 1 - (a & b); }
inline int NOR(int a, int b)  { return 1 - (a | b); }
inline int XOR(int a, int b)  { return a ^ b; }
inline int XNOR(int a, int b) { return 1 - (a ^ b); }

}  // namespace sim

using namespace sim;
