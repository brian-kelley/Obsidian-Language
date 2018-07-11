#ifndef DATAFLOW_H
#define DATAFLOW_H

#include "IR.hpp"

struct Variable;

//Forward and backward liveness analysis
//A variable is "live" from its first definition to
//its last use, inclusive (last use is traditional "liveness",
//forward use here is more like reaching defs)
namespace Liveness
{
  struct LiveSet
  {
    //Construct empty live set
    LiveSet() {}
    //Construct live set as intersection of forward and backward live sets
    LiveSet(LiveSet* fw, LiveSet* bw);
    void insertVars(IR::BasicBlock* bb, set<Variable*> vars);
    void intersectVars(IR::BasicBlock* bb, set<Variable*> vars);
    map<IR::BasicBlock*, set<Variable*>> live;
  };
  extern map<IR::SubroutineIR*, LiveSet*> liveSets;
  void buildAllLivesets();
}

#endif

