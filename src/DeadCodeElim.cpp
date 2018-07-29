#include "DeadCodeElim.hpp"

using namespace IR;

bool deadCodeElim(SubroutineIR* subr)
{
  //replace cond jumps with constant conditions with regular jumps
  bool update = false;
  for(size_t i = 0; i < subr->stmts.size(); i++)
  {
    if(auto condJump = dynamic_cast<CondJump*>(subr->stmts[i]))
    {
      if(auto boolConst = dynamic_cast<BoolConstant*>(condJump->cond))
      {
        update = true;
        if(boolConst->value)
        {
          //never taken (does nothing)
          subr->stmts[i] = nop;
        }
        else
        {
          //always taken (just a regular jump)
          subr->stmts[i] = new Jump(condJump->taken);
        }
      }
    }
  }
  size_t stmtsBefore = subr->stmts.size();
  //do a breadth-first search of reachability from the first
  //BB to delete all unreachable in one pass
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
        visitQueue.push(neighbor);
    }
  }
  for(auto bb : subr->blocks)
  {
    if(blockVisits[bb] == NOT_VISITED)
    {
      for(int i = bb->start; i < bb->end; i++)
      {
        subr->stmts[i] = nop;
      }
    }
  }
  //then delete NOPs and rebuild CFG
  subr->buildCFG();
  //IR changed if # stmts changed
  return update || stmtsBefore != subr->stmts.size();
}

