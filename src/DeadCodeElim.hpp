#ifndef DEADCODE_H
#define DEADCODE_H

#include "IR.hpp"

//go through CFG and replace conditional jumps with
//regular jumps when condition is known at compile time
bool deadCodeElim(IR::SubroutineIR* subr);
//delete unreachable instructions
bool deleteUnreachable(IR::SubroutineIR* subr);

#endif

