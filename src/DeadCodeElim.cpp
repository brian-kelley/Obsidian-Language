#include "DeadCodeElim.hpp"

void deadCodeElim(SubroutineIR* subr)
{
  //replace unreachable BBs with no-ops
  for(size_t i = 0; i < subr->blocks.size(); i++)
  {
    BasicBlock* bb = subr->blocks[i];
    if(i > 0 && bb->out.size() == 0)
    {
      for(int j = bb->start; j < bb->end; j++)
      {
        bb->stmts[j] = nop;
      }
    }
  }
  //remove no-ops from IR, and rebuild CFG
  auto newEnd = std::remove_if(
      subr->stmts.begin(), subr->stmts.end(),
      [](StatementIR* s){return dynamic_cast<Nop*>(s);});
  subr->stmts.erase(newEnd, subr->stmts.end());
  subr->buildCFG();
}

