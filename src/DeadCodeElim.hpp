#ifndef DEADCODE_H
#define DEADCODE_H

#include "IR.hpp"

//go through CFG and replace conditional jumps with
//regular jumps when condition is known at compile time
//
//then delete all unreachable instructions
bool deadCodeElim(IR::SubroutineIR* subr);

#endif

