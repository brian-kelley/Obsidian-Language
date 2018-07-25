#ifndef CONSTANT_PROP_H
#define CONSTANT_PROP_H

#include "IR.hpp"

void determineGlobalConstants();
//Constant folding evaluates all arithmetic on constants
bool constantFold(IR::SubroutineIR* subr);
//Constant propagation decides which local variables have
//constant values at each statement, and replaces their
//usage with constants
//(first just within BBs, then across whole subroutine)
bool constantPropagation(IR::SubroutineIR* subr);

#endif

