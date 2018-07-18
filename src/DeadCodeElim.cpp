#include "DeadCodeElim.hpp"

using namespace IR;

bool deadCodeElim(SubroutineIR* subr)
{
  //if a BB's only incoming edge is the previous BB,
  //then they can be merged
  //Replace label with no-op, and jump/condjump in previous block (if any)
  bool update = false;
  for(size_t i = 1; i < subr->blocks.size(); i++)
  {
    BasicBlock* thisBlock = subr->blocks[i];
    BasicBlock* prevBlock = subr->blocks[i - 1];
    if(prevBlock->out.size() == 1 && thisBlock->in.size() == 1 && thisBlock->in[0] == prevBlock)
    {
      //get statements before and after BB boundary
      auto& stmtBefore = subr->stmts[prevBlock->end - 1];
      auto& stmtAfter = subr->stmts[prevBlock->end];
      if(dynamic_cast<Jump*>(stmtBefore) || dynamic_cast<CondJump*>(stmtBefore))
      {
        stmtBefore = nop;
        update = true;
      }
      if(dynamic_cast<Label*>(stmtAfter))
      {
        stmtAfter = nop;
        update = true;
      }
    }
  }
  for(size_t i = 0; i < subr->blocks.size(); i++)
  {
    BasicBlock* bb = subr->blocks[i];
    if(i > 0 && bb->out.size() == 0)
    {
      for(int j = bb->start; j < bb->end; j++)
      {
        subr->stmts[j] = nop;
      }
    }
  }
  size_t oldNumStmts = subr->stmts.size();
  //remove no-ops from IR, and rebuild the CFG
  auto newEnd = std::remove_if(
      subr->stmts.begin(), subr->stmts.end(),
      [](StatementIR* s){return dynamic_cast<Nop*>(s);});
  subr->stmts.erase(newEnd, subr->stmts.end());
  update = update || subr->stmts.size() != oldNumStmts;
  if(update)
  {
    subr->buildCFG();
  }
  return update;
}

