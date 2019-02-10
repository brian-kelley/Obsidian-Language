#include "DeadCodeElim.hpp"

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

void deadStoreElim(IR::SubroutineIR* subr)
{
  //Compute live sets and reaching definitions
  
}

