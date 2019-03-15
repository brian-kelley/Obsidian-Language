#include "CallGraph.hpp"
#include "IR.hpp"

using namespace IR;

//The set of all subroutines/external subroutines
//which are possibly reachable through indirect calls
//
//These are looked up by CallableType, since it is assumed
//that any expression of a CallableType can refer to any
//callable expr that has been used outside of a CallExpr.
unordered_map<CallableType*, set<Callable>,
  TypeHash, TypeEqual> indirectReachables;

CallGraph callGraph;

//Need to provide comparison operators to use a set/map of them
//Easy because Subroutine, ExternalSubroutine are pointer-identifiable
bool operator==(const Callable& lhs, const Callable& rhs)
{
  return lhs.s == rhs.s;
}

bool operator<(const Callable& lhs, const Callable& rhs)
{
  return lhs.s < rhs.s;
}

void CallGraph::addNode(Callable c)
{
  if(nodes.find(c) == nodes.end())
    nodes[c] = CGNode();
}

void CallGraph::addEdge(IR::SubroutineIR* s, CallableType* indirect)
{
  addNode(Callable(s));
  nodes[Callable(s)].outIndirect.insert(indirect);
}

void CallGraph::addEdge(IR::SubroutineIR* s, Callable direct)
{
  addNode(Callable(s));
  nodes[Callable(s)].outDirect.insert(direct);
}

void CallGraph::addEdge(ExternalSubroutine* es, CallableType* indirect)
{
  addNode(Callable(es));
  nodes[Callable(es)].outIndirect.insert(indirect);
}

void determineIndirectReachable()
{
  //For subroutines and external subroutines to
  //be reachable through an indirect call, they MUST
  //appear somewhere as an expression (so find all such
  //SubroutineExprs)
  for(auto& s : ir)
  {
    for(auto& stmt : s->stmts)
    {
      //note: only need to check the statement types that
      //let expressions get assigned to a variable/parameter (only place
      //where indirect calls are possible)
      if(auto a = dynamic_cast<AssignIR*>(stmt))
      {
        gatherIndirectCallables(a->dst);
        gatherIndirectCallables(a->src);
      }
      else if(auto ev = dynamic_cast<EvalIR*>(stmt))
      {
        gatherIndirectCallables(ev->eval);
      }
      else if(auto cj = dynamic_cast<CondJump*>(stmt))
      {
        gatherIndirectCallables(cj->cond);
      }
      else if(auto r = dynamic_cast<ReturnIR*>(stmt))
      {
        if(r->expr)
          gatherIndirectCallables(r->expr);
      }
    }
  }
}

void gatherIndirectCallables(Expression* e)
{
  if(auto subExpr = dynamic_cast<SubroutineExpr*>(e))
  {
    //the most important case: a subroutine
    //was used in expression, not in a CallExpr
    if(subExpr->subr)
      registerIndirectCallable(Callable(subExpr->subr));
    else
      registerIndirectCallable(Callable(subExpr->exSubr));
  }
  else if(auto cl = dynamic_cast<CompoundLiteral*>(e))
  {
    for(auto m : cl->members)
    {
      gatherIndirectCallables(m);
    }
  }
  else if(auto in = dynamic_cast<Indexed*>(e))
  {
    gatherIndirectCallables(in->group);
    gatherIndirectCallables(in->index);
  }
  else if(auto ae = dynamic_cast<AsExpr*>(e))
  {
    gatherIndirectCallables(ae->base);
  }
  else if(auto ie = dynamic_cast<IsExpr*>(e))
  {
    gatherIndirectCallables(ie->base);
  }
  else if(auto call = dynamic_cast<CallExpr*>(e))
  {
    //Here is where SubroutineExprs in calls are excluded
    if(auto subrExpr = dynamic_cast<SubroutineExpr*>(call->callable))
    {
      if(subrExpr->thisObject)
        gatherIndirectCallables(subrExpr->thisObject);
    }
    for(auto a : call->args)
    {
      gatherIndirectCallables(a);
    }
  }
  else if(auto conv = dynamic_cast<Converted*>(e))
  {
    gatherIndirectCallables(conv->value);
  }
}

void registerIndirectCallable(Callable c)
{
  CallableType* t = nullptr;
  if(auto subr = dynamic_cast<Subroutine*>(c.s))
  {
    t = subr->type;
  }
  else
  {
    auto exSubr = dynamic_cast<ExternalSubroutine*>(c.s);
    t = exSubr->type;
  }
  auto it = indirectReachables.find(t);
  if(it == indirectReachables.end())
  {
    set<Callable> initial;
    initial.insert(c);
    indirectReachables[t] = initial;
  }
  else
  {
    it->second.insert(c);
  }
}

void CallGraph::rebuild()
{
  nodes.clear();
  determineIndirectReachable();
  //First, create all the CG nodes for normal subroutines.
  //This includes outgoing edges for regular direct calls,
  //and all possible indirect calls
  for(auto& subrIR : ir)
  {
    //for each statement, find call instructions
    for(auto stmt : subrIR->stmts)
    {
      CallExpr* ce = nullptr;
      if(auto ev = dynamic_cast<EvalIR*>(stmt))
      {
        ce = dynamic_cast<CallExpr*>(ev->eval);
      }
      else if(auto as = dynamic_cast<AssignIR*>(stmt))
      {
        ce = dynamic_cast<CallExpr*>(as->src);
      }
      if(ce)
      {
        if(auto subExpr = dynamic_cast<SubroutineExpr*>(ce->callable))
        {
          if(subExpr->subr)
            callGraph.addEdge(subr, Callable(subExpr->subr));
          else
            callGraph.addEdge(subr, Callable(subExpr->exSubr));
        }
        else
        {
          //indirect call: use the type of callable
          callGraph.addEdge(subr, (CallableType*) ce->callable->type);
        }
      }
    }
  }
  //Then, determine all possible calls by external subroutines.
  //Since the only Onyx expressions visible to external subroutines
  //are their parameters, those are the only place where they can
  //get subroutines.
  Scope::walk([&](Scope* s)
  {
    for(auto n : s->names)
    {
      Name& name = n.second;
      if(name.kind == Name::EXTERN_SUBR)
      {
        auto exSubr = (ExternalSubroutine*) name.item;
        set<Type*> deps;
        for(auto param : exSubr->type->paramTypes)
        {
          param->dependencies(deps);
        }
        for(auto t : deps)
        {
          if(auto ct = dynamic_cast<CallableType*>(t))
            callGraph.addEdge(exSubr, ct);
        }
      }
    }
  });
}

void CallGraph::dump(string path)
{
  Dotfile dot("Global Call Graph");
  //create the first node, for main
  dot.write(path);
}

