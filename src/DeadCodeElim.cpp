#include "DeadCodeElim.hpp"
#include "Dataflow.hpp"
#include "Variable.hpp"

using namespace IR;

bool deadCodeElim(SubroutineIR* subr)
{
  //replace cond jumps with constant conditions with regular jumps,
  //and remove always-true and always-false assertions
  bool updatePhase1 = false;
  for(size_t i = 0; i < subr->stmts.size(); i++)
  {
    if(auto condJump = dynamic_cast<CondJump*>(subr->stmts[i]))
    {
      if(auto boolConst = dynamic_cast<BoolConstant*>(condJump->cond))
      {
        updatePhase1 = true;
        if(boolConst->value)
        {
          //branch never taken (does nothing)
          subr->stmts[i] = nop;
        }
        else
        {
          //always taken (just a regular jump)
          subr->stmts[i] = new Jump(condJump->taken);
        }
      }
    }
    else if(auto asi = dynamic_cast<AssertionIR*>(subr->stmts[i]))
    {
      if(auto constAsserted = dynamic_cast<BoolConstant*>(asi->asserted))
      {
        if(constAsserted->value)
        {
          //assert(true) is a no-op
          subr->stmts[i] = nop;
          updatePhase1 = true;
        }
        else
        {
          //assert(false) will always fail,
          //so tell user about it at compile time
          errMsgLoc(asi->asserted, "asserted condition always false");
        }
      }
    }
  }
  if(updatePhase1)
  {
    subr->buildCFG();
  }
  //do a breadth-first search of reachability from the first
  //BB to delete all unreachable code in one pass
  enum
  {
    NOT_VISITED,
    QUEUED,
    VISITED
  };
  map<BasicBlock*, char> blockVisits;
  for(auto bb : subr->blocks)
  {
    blockVisits[bb] = NOT_VISITED;
  }
  queue<BasicBlock*> visitQueue;
  visitQueue.push(subr->blocks[0]);
  blockVisits[subr->blocks[0]] = QUEUED;
  for(size_t i = 1; i < subr->blocks.size(); i++)
  {
    blockVisits[subr->blocks[i]] = NOT_VISITED;
  }
  while(visitQueue.size())
  {
    BasicBlock* process = visitQueue.front();
    visitQueue.pop();
    blockVisits[process] = VISITED;
    for(auto neighbor : process->out)
    {
      if(blockVisits[neighbor] == NOT_VISITED)
      {
        visitQueue.push(neighbor);
        blockVisits[neighbor] = QUEUED;
      }
    }
  }
  bool updatePhase2 = false;
  for(auto bb : subr->blocks)
  {
    if(blockVisits[bb] == NOT_VISITED)
    {
      updatePhase2 = true;
      for(int i = bb->start; i < bb->end; i++)
      {
        subr->stmts[i] = nop;
      }
    }
  }
  //then delete NOPs and rebuild CFG
  if(updatePhase2)
    subr->buildCFG();
  return updatePhase1 || updatePhase2;
}

void deadStoreElim(SubroutineIR* subr)
{
  ReachingDefs reaching(subr);
  int numAssigns = reaching.allAssigns.size();
  Liveness live(subr);
  int numVars = live.allVars.size();
  //Record which defs actually reach a usage (unused defs may be deleted)
  vector<bool> defsUsed(numAssigns, false);
  //Within each block, compute live set entry to each stmt (backwards)
  for(size_t b = 0; b < subr->blocks.size(); b++)
  {
    //compute the live and reaching sets at the beginning of each statement
    BasicBlock* block = subr->blocks[b];
    //First phase: delete assignments modifying dead variables.
    {
      LiveSet stmtLive(numVars, false);
      for(auto succ : block->out)
      {
        unionMeet(stmtLive, live.live[succ->index]);
      }
      for(int i = block->end - 1; i >= block->start; i--)
      {
        live.transfer(stmtLive, subr->stmts[i]);
        if(auto assign = dynamic_cast<AssignIR*>(subr->stmts[i]))
        {
          Variable* v = assign->dst->getWrite();
          if(!live.isLive(stmtLive, v))
          {
            cout << "Deleted dead store: " << subr->stmts[i] << '\n';
            if(assign->src->hasSideEffects())
              subr->stmts[i] = new EvalIR(assign->src);
            else
              subr->stmts[i] = nop;
          }
        }
      }
    }
    //Second phase: for every usage of V, flag defs of V reaching usage.
    {
      ReachingSet stmtReach(numAssigns, false);
      for(auto pred : block->in)
        unionMeet(stmtReach, reaching.reaching[pred->index]);
      for(int i = block->start; i < block->end; i++)
      {
        reaching.transfer(stmtReach, subr->stmts[i]);
        set<Variable*> used;
        subr->stmts[i]->getReads(used);
        for(int j = 0; j < numAssigns; j++)
        {
          //(optimization) test as few defs as possible
          if(!defsUsed[j] && stmtReach[j])
          {
            Variable* defWrite = reaching.allAssigns[j]->dst->getWrite();
            if(used.find(defWrite) != used.end())
              defsUsed[j] = true;
          }
        }
      }
    }
  }
  //Finally, remove all unused assignments to locals.
  for(int i = 0; i < numAssigns; i++)
  {
    AssignIR* assign = reaching.allAssigns[i];
    if(!defsUsed[i] && assign->dst->getWrite()->isLocal())
    {
      cout << "Deleted unused assignment: " << subr->stmts[assign->intLabel] << '\n';
      if(assign->src->hasSideEffects())
        subr->stmts[assign->intLabel] = new EvalIR(assign->src);
      else
        subr->stmts[assign->intLabel] = nop;
    }
  }
  //Delete nops and rebuild CFG
  subr->buildCFG();
}

