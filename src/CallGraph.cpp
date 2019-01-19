#include "CallGraph.hpp"
#include "IR.hpp"

using namespace IR;

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

//Get the set of indirectly-reachable callables with the given type
//Must assume that any indirect call of a callable with type ct could
//refer to any of these callables
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

//Get the set of Callables (effectively, SubroutineExprs) contained
//in the expression
set<Callable> getExpressionCallables(Expression* e)
{
  set<Callable> c;
  if(auto subExpr = dynamic_cast<SubroutineExpr*>(e))
  {
    //the most important case: a subroutine
    //used in expression outside call
    if(subExpr->subr)
      c.insert(subExpr->subr);
    else
      c.insert(subExpr->exSubr);
  }
  else if(auto cl = dynamic_cast<CompoundLiteral*>(e))
  {
    for(auto m : cl->members)
    {
      auto temp = getExpressionCallables(m);
      c.insert(temp.begin(), temp.end());
    }
  }
  else if(auto in = dynamic_cast<Indexed*>(e))
  {
    return getExpressionCallables(in->group);
  }
  else if(auto ae = dynamic_cast<AsExpr*>(e))
  {
    return getExpressionCallables(ae->base);
  }
  else if(auto ie = dynamic_cast<IsExpr*>(e))
  {
    return getExpressionCallables(ie->base);
  }
  else if(auto call = dynamic_cast<CallExpr*>(e))
  {
    for(auto a : call->args)
    {
      auto temp = getExpressionCallables(a);
      c.insert(temp.begin(), temp.end());
    }
  }
  else if(auto conv = dynamic_cast<Converted*>(e))
  {
    return getExpressionCallables(conv->value);
  }
  return c;
}

//trivial helper to just add to indirect reachables whatever
//SubroutineExprs appear in an expression
void addIndirectReachables(Expression* e)
{
  auto temp = getExpressionCallables(e);
  indirectReachable.insert(temp.begin(), temp.end());
}

void determineIndirectReachable()
{
  //For subroutines and external subroutines to
  //be reachable through an indirect call, they MUST
  //appear somewhere as an expression (so find all such
  //SubroutineExprs)
  for(auto& s : IR::ir)
  {
    for(auto& stmt : s.second->stmts)
    {
      //note: only need to check the statement types that
      //let expressions get assigned to a variable/parameter (only place
      //where indirect calls are possible)
      if(auto a = dynamic_cast<AssignIR*>(stmt))
      {
        addIndirectReachables(a->dst);
        addIndirectReachables(a->src);
      }
      else if(auto ev = dynamic_cast<EvalIR*>(stmt))
      {
        addIndirectReachables(ev->eval);
      }
      else if(auto cj = dynamic_cast<CondJump*>(stmt))
      {
        addIndirectReachables(cj->cond);
      }
      else if(auto r = dynamic_cast<ReturnIR*>(stmt))
      {
        if(r->expr)
          addIndirectReachables(r->expr);
      }
    }
  }
}

//Given an expression, return all possible call targets
//determineIndirectReachable() must have been called first
static set<Callable> getExprCalls(Expression* e)
{
  set<Callable> c;
  if(auto cl = dynamic_cast<CompoundLiteral*>(e))
  {
    for(auto m : cl->members)
    {
      auto temp = getExprCalls(m);
      c.insert(temp.begin(), temp.end());
    }
  }
  else if(auto ua = dynamic_cast<UnaryArith*>(e))
  {
    auto temp = getExprCalls(ua->expr);
    c.insert(temp.begin(), temp.end());
  }
  else if(auto ba = dynamic_cast<BinaryArith*>(e))
  {
    auto temp = getExprCalls(ba->lhs);
    c.insert(temp.begin(), temp.end());
    temp = getExprCalls(ba->rhs);
    c.insert(temp.begin(), temp.end());
  }
  else if(auto ind = dynamic_cast<Indexed*>(e))
  {
    auto temp = getExprCalls(ind->group);
    c.insert(temp.begin(), temp.end());
    temp = getExprCalls(ind->index);
    c.insert(temp.begin(), temp.end());
  }
  else if(auto al = dynamic_cast<ArrayLength*>(e))
  {
    auto temp = getExprCalls(al->array);
    c.insert(temp.begin(), temp.end());
  }
  else if(auto as = dynamic_cast<AsExpr*>(e))
  {
    auto temp = getExprCalls(as->base);
    c.insert(temp.begin(), temp.end());
  }
  else if(auto is = dynamic_cast<IsExpr*>(e))
  {
    auto temp = getExprCalls(is->base);
    c.insert(temp.begin(), temp.end());
  }
  else if(auto call = dynamic_cast<CallExpr*>(e))
  {
    //first, process the args
    for(auto arg : call->args)
    {
      auto temp = getExprCalls(arg);
      c.insert(temp.begin(), temp.end());
    }
    //then, add outgoing edge(s) for this call
    if(auto direct = dynamic_cast<SubroutineExpr*>(call->callable))
    {
      //is a direct call, only one edge to add
      if(direct->subr)
        c.insert(direct->subr);
      else
        c.insert(direct->exSubr);
    }
    else
    {
      //indirect call
      auto temp = getReachableCallables((CallableType*) call->callable->type);
      c.insert(temp.begin(), temp.end());
    }
  }
  else if(auto conv = dynamic_cast<Converted*>(e))
  {
    auto temp = getExprCalls(conv->value);
    c.insert(temp.begin(), temp.end());
  }
  //else: no child expressions so no calls
  return c;
}

void buildCallGraph()
{
  determineIndirectReachable();
  //First, create all the CG nodes for normal subroutines.
  //This includes outgoing edges for regular direct calls,
  //and all possible indirect calls
  for(auto& s : IR::ir)
  {
    auto subr = s.second;
    CGNode* node = new CGNode;
    node->c = s.first;
    //for each statement, find call instructions (both in stmts and exprs)
    for(auto stmt : subr->stmts)
    {
      if(auto ai = dynamic_cast<AssignIR*>(stmt))
      {
        auto temp = getExprCalls(ai->dst);
        node->out.insert(temp.begin(), temp.end());
        temp = getExprCalls(ai->src);
        node->out.insert(temp.begin(), temp.end());
      }
      else if(auto ev = dynamic_cast<EvalIR*>(stmt))
      {
        auto temp = getExprCalls(ev->eval);
        node->out.insert(temp.begin(), temp.end());
      }
      else if(auto cj = dynamic_cast<CondJump*>(stmt))
      {
        auto temp = getExprCalls(cj->cond);
        node->out.insert(temp.begin(), temp.end());
      }
      else if(auto ri = dynamic_cast<ReturnIR*>(stmt))
      {
        if(ri->expr)
        {
          auto temp = getExprCalls(ri->expr);
          node->out.insert(temp.begin(), temp.end());
        }
      }
      else if(auto pi = dynamic_cast<PrintIR*>(stmt))
      {
        auto temp = getExprCalls(pi->expr);
        node->out.insert(temp.begin(), temp.end());
      }
      else if(auto asi = dynamic_cast<AssertionIR*>(stmt))
      {
        auto temp = getExprCalls(asi->asserted);
        node->out.insert(temp.begin(), temp.end());
      }
      //else: statement uses no expressions and can't call anything
    }
    callGraph.nodes[s.first] = node;
  }
  //Then, determine all possible calls by external subroutines:
  //since external subroutines can't access global vars, they
  //may only call first-class subroutines passed in through arguments.
  //TODO!!!
  /*
  for(auto es : allExSubrs)
  {
    CGNode* node = new CGNode;
    for(auto argType : es->type->argTypes)
    {
      auto dependencyTypes = argType->dependencies();
      for(auto t : dependencyTypes)
      {
        if(auto ct = dynamic_cast<CallableType*>(t))
        {
          //a callable type can be passed to external subroutine here
          auto temp = getReachableCallables(ct);
          node->out.insert(temp.begin(), temp.end());
        }
      }
    }
    callGraph.nodes[es] = node;
  }
  */
}

