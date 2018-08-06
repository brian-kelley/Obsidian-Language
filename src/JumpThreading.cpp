#include "JumpThreading.hpp"
#include <algorithm>

using namespace IR;

bool simplifyCFG(SubroutineIR* subr)
{
  bool update = true;
  bool anyUpdate = false;
  //for any two BasicBlocks A,B, if:
  //  -A immediately before B in statement numbering
  //  -A's only outgoing edge goes to B
  //  -B's only incoming edge comes from A
  //then merge A and B (delete the label which is the leader of B)
  while(update)
  {
    update = false;
    for(size_t i = 0; i < subr->blocks.size() - 1; i++)
    {
      auto prevBB = subr->blocks[i];
      auto nextBB = subr->blocks[i + 1];
      if(prevBB->out.size() == 1 && prevBB->out[0] == nextBB &&
          nextBB->in.size() == 1 && nextBB->in[0] == prevBB)
      {
        cout << "Merging basic blocks at statement " << prevBB->end << '\n';
        update = true;
        int boundary = nextBB->start;
        if(dynamic_cast<Jump*>(subr->stmts[boundary - 1]))
          subr->stmts[boundary - 1] = nop;
        if(dynamic_cast<Label*>(subr->stmts[boundary]))
          subr->stmts[boundary] = nop;
        //extend prevBB
        prevBB->end = nextBB->end;
        prevBB->out = nextBB->out;
        for(auto nextOut : nextBB->out)
        {
          std::replace(nextOut->in.begin(), nextOut->in.end(), nextBB, prevBB);
        }
        //delete nextBB by shifting down all later blocks
        for(size_t j = i; j < subr->blocks.size() - 1; j++)
        {
          subr->blocks[j] = subr->blocks[j + 1];
        }
        subr->blocks.pop_back();
      }
    }
    anyUpdate = anyUpdate || update;
  }
  if(anyUpdate)
    subr->buildCFG();
  return anyUpdate;
}

bool jumpThreading(SubroutineIR* subr)
{
  bool update = false;
  for(size_t i = 0; i < subr->stmts.size(); i++)
  {
    auto stmt = subr->stmts[i];
    if(auto jump = dynamic_cast<Jump*>(stmt))
    {
      int target = jump->dst->intLabel;
      //is the jump effectively a no-op?
      //(jumping forward over no active statements)
      bool jumpNop = false;
      if(target > i)
        jumpNop = true;
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
        //replace the jump with nop
        subr->stmts[i] = nop;
        continue;
      }
      //otherwise, can control flow be traced to a different label
      //to reduce number of jumps taken?
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
    subr->buildCFG();
  /*
  if(simplifyCFG(subr))
  {
    subr->buildCFG();
    update = true;
  }
  */
  return update;
}

