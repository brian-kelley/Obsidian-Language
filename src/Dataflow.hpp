/*
#ifndef DATAFLOW_H
#define DATAFLOW_H

#include "IR.hpp"

struct Variable;

using IR::SubroutineIR;
using IR::StatementIR;
using IR::AssignIR;

//Reaching defs at a given stmt is the set of definitions
//(AssignIRs) for each local variable, where there might be
//no intervening definitions.
//
//Reaching defs = forward dataflow, meet operator is union.
//Locally, defs are both generated and killed by new defs of the same var.
typedef vector<bool> ReachingSet;

struct ReachingDefs
{
  ReachingDefs(SubroutineIR* subr);
  void transfer(ReachingSet& r, StatementIR* stmt);
  //Table of all assignments and corresponding indices in reaching[k]
  unordered_map<AssignIR*, int> assignTable;
  //Reaching-def set for each block
  vector<AssignIR*> allAssigns;
  vector<ReachingSet> reaching;
};

typedef vector<bool> LiveSet;

//A variable is live before and including its last use.
struct Liveness
{
  Liveness(SubroutineIR* subr);
  void transfer(LiveSet& r, StatementIR* stmt);
  bool isLive(LiveSet& l, Variable* v);
  //Tables of all local variables
  vector<Variable*> allVars;
  unordered_map<Variable*, int> varTable;
  //Live sets at the beginning of each 
  vector<LiveSet> live;
};

//Bitwise union (or) operation - for both liveness and reaching
void unionMeet(vector<bool>& into, vector<bool>& from);

#endif
*/

