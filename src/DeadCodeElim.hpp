#ifndef DEADCODE_H
#define DEADCODE_H

#include "IR.hpp"

//Go through CFG and replace conditional jumps with
//regular jumps if the condition value is known at compile time
//
//Then delete all unreachable instructions
bool deadCodeElim(IR::SubroutineIR* subr);

//Do liveness analysis, and delete assignments to dead variables
//(still evaluate RHS if it has side effects).
//Finally, completely delete local variables that are never alive
//It only makes sense to run this once, so it doesn't return true for updates.
void deadStoreElim(IR::SubroutineIR* subr);

#endif

