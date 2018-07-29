#include "JumpThreading.hpp"

using namespace IR;

bool jumpThreading(SubroutineIR* subr)
{
  bool update = false;
  for(size_t i = 0; i < subr->stmts.size(); i++)
  {
    auto stmt = subr->stmts[i];
    if(auto jump = dynamic_cast<Jump*>(stmt))
    {
      int target = jump->dst->intLabel;
      bool jumpNop = true;
      for(int j = i + 1; j < target; j++)
      {
        if(!dynamic_cast<Nop*>(subr->stmts[j]))
        {
          jumpNop = false;
          break;
        }
      }
      if(jumpNop)
      {
        update = true;
        subr->stmts[i] = nop;
        continue;
      }
      int targetInst = target + 1;
      while(targetInst < subr->stmts.size())
      {
        if(dynamic_cast<Label*>(subr->stmts[targetInst]))
        {
          target = targetInst;
          targetInst++;
        }
        else if(dynamic_cast<Nop*>(subr->stmts[targetInst]))
        {
          targetInst++;
        }
        else if(auto nextJump = dynamic_cast<Jump*>(subr->stmts[targetInst]))
        {
          target = nextJump->dst->intLabel;
          targetInst = target + 1;
        }
        else
        {
          break;
        }
      }
      //update the original jump target
      if(jump->dst->intLabel != target)
        update = true;
      jump->dst = (Label*) subr->stmts[target];
    }
    else if(auto condJump = dynamic_cast<CondJump*>(stmt))
    {
      int takenTaget = condJump->taken->intLabel;
      int notTakenTarget = i + 1;
      if(takenTaget == notTakenTarget)
      {
        //can safely delete the condjump with nop
        subr->stmts[i] = nop;
        update = true;
      }
    }
  }
  if(update)
  {
    subr->buildCFG();
  }
  return update;
}

