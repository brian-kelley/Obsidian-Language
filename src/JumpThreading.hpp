#ifndef JUMP_THREAD_H
#define JUMP_THREAD_H

#include "IR.hpp"

bool simplifyCFG(IR::SubroutineIR* subr);
bool jumpThreading(IR::SubroutineIR* subr);

#endif

