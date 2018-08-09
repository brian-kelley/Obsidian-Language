#include "CallGraph.hpp"

//The set of all subroutines/external subroutines
//which are possibly reachable through indirect calls
set<Callable> indirectReachable;

CallGraph callGraph;

bool operator==(const Callable& lhs, const Callable& rhs)
{
  return lhs.subr == rhs.subr && lhs.exSubr == rhs.exSubr;
}

bool operator<(const Callable& lhs, const Callable& rhs)
{
  if(lhs.subr < rhs.subr)
    return true;
  else if(lhs.subr > rhs.subr)
    return false;
  return lhs.exSubr < rhs.exSubr;
}

static set<Callable> getReachableCallables(CallableType* ct)
{
  set<Callable> reach;
  for(auto& c : indirectReachable)
  {
    if(c.type() == ct)
      reach.insert(c);
  }
  return reach;
}

static set<Callable> getExpressionCallables(Expression* e)
{
  set<Variable*> writes;
  if(auto cl = dynamic_cast<CompoundLiteral*>(e))
  {
    for(auto m : cl->members)
    {
      auto mw = getExpressionWrites(m, lhs);
      writes.insert(mw.begin(), mw.end());
    }
  }
  else if(auto ua = dynamic_cast<UnaryArith*>(e))
  {
    return getExpressionWrites(na->expr, false);
  }
  else if(auto ba = dynamic_cast<BinaryArith*>(e))
  {
    auto w = getExpressionWrites(ba->lhs, false);
    writes.insert(w.begin(), w.end());
    w = getExpressionWrites(ba->lhs, false);
    writes.insert(w.begin(), w.end());
  }
  else if(auto in = dynamic_cast<Indexed*>(e))
  {
    //group can be an lvalue, but index cannot
    auto w = getExpressionWrites(in->group, lhs);
    writes.insert(w.begin(), w.end());
    w = getExpressionWrites(in->index, false);
    writes.insert(w.begin(), w.end());
  }
  else if(auto al = dynamic_cast<ArrayLength*>(e))
  {
    return getExpressionWrites(al->array, false);
  }
  else if(auto ae = dynamic_cast<AsExpr*>(e))
  {
    return getExpressionWrites(ae->base, false);
  }
  else if(auto ie = dynamic_cast<IsExpr*>(e))
  {
    return getExpressionWrites(ie->base, false);
  }
  else if(auto call = dynamic_cast<CallExpr*>(e))
  {
    //need to use the call graph to determine possibly modified variables
  }
  else if(auto var = dynamic_cast<VarExpr*>(e))
  {
  }
  else if(auto conv = dynamic_cast<Converted*>(e))
  {
  }
  return writes;
}

void determineIndirectReachable()
{
  //For subroutines and external subroutines to
  //be reachable through an indirect call, they MUST
  //appear somewhere as an expression (so find all such
  //SubroutineExprs)
  for(auto& s : IR::ir)
  {
  }
}

