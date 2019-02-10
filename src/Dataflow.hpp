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
namespace ReachingDefs
{
  typedef set<AssignIR*> ReachingSet;
  vector<ReachingSet> compute(SubroutineIR* subr);
  void transfer(ReachingSet& r, StatementIR* stmt);
  void meet(ReachingSet& into, ReachingSet& from);
  bool operator==(const ReachingSet& r1, const ReachingSet& r2);
}

//A variable is live before and including its last use.
namespace Liveness
{
  typedef set<Variable*> LiveSet;
  vector<LiveSet> compute(SubroutineIR* subr);
  void transfer(LiveSet& r, StatementIR* stmt);
  void meet(LiveSet& into, LiveSet& from);
  bool operator==(const LiveSet& l1, const LiveSet& l2);
}

#endif

