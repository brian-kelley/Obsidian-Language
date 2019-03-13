#include "Memory.hpp"
  /*

bool modifiedParamComputed = false;
//Table of all parameters in whole function.
//The bool value is whether the parameter value is modified
//somewhere in its owning subroutine.
//Used for determining whether it's possible to borrow the
//passed in variable, instead of deep copy.
map<Variable*, bool> paramsModified;

static void determineModifiedParameters()
{
  //initialize table of all params
  for(auto& i : IR::ir)
  {
    for(auto& a : i.first->args.size())
      paramsModified[a] = false;
  }
  bool update = true;
  while(update)
  {
    for(auto& i : IR::ir)
    {
      //IR invariant: all calls are either RHS of assign,
      //or an Eval
      for(auto stmt : i.second->stmts)
      {
        if(auto assign = dynamic_cast<AssignIR*>(stmt))
        {
        }
        else if(auto eval = dynamic_cast<EvalIR*>(stmt))
        {
          if(auto call = dynamic_cast<CallExpr*>(eval->eval))
          {
          }
        }
      }
    }
  }
}

bool parameterModified(Variable* p)
{
  if(!modifiedParamComputed)
  {
    determineModifiedParameters();
    modifiedParamComputed = true;
  }
  assert(p->isParameter());
  Subroutine* subr = p->scope->node.get<Subroutine*>();
  SubroutineIR* subrIR = IR::ir[subr];

}

void Memory::computeParameterMods()
{
}

  */
