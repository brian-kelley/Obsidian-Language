#ifndef DATAFLOW_H
#define DATAFLOW_H

#include "IR.hpp"

struct Variable;

namespace Liveness
{
  struct LiveSet
  {
    void insertVars(IR::BasicBlock* bb, set<Variable*> vars);
    map<IR::BasicBlock*, set<Variable*>> live;
  };
  extern map<IR::SubroutineIR*, LiveSet*> liveSets;
  void buildAllLivesets();
}

#endif

