#include "CFG.hpp"
#include "Common.hpp"
#include "Scope.hpp"
#include "Subroutine.hpp"

CFG::CFG(SubroutineIR* s)
{
  subr = s;
  //basic block boundaries:
  //-at label
  //-after jump
  //-after return
  size_t bbStart = 0;
  for(size_t i = 0; i < s->stmts.size(); i++)
  {
    if(dynamic_cast<Label*>(s->stmts[i]))
    {
    }
  }
}

