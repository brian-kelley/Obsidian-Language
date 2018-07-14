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
    //part of the liveness is just whether each subroutine argument gets modified
    //if an argument is never modified, it's never necessary to deep-copy it in the caller
    vector<bool> argsModified;
  };
  extern map<IR::SubroutineIR*, LiveSet*> liveSets;
  void buildAllLivesets();
}

#endif

