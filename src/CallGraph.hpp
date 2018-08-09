#ifdef CALL_GRAPH_H
#define CALL_GRAPH_H

#include "Common.hpp"
#include "IR.hpp"

struct Callable
{
  Callable()
  {
    subr = nullptr;
    exSubr = nullptr;
  }
  Callable(Subroutine* s) : subr(s), exSubr(nullptr) {}
  Callable(ExternalSubroutine* es) : subr(nullptr), exSubr(es) {}
  Callable(const Callable& other) : subr(other.subr), exSubr(other.exSubr) {}
  CallableType* type()
  {
    if(subr)
      return subr->type;
    return exSubr->type;
  }
  Subroutine* subr;
  ExternalSubroutine* exSubr;
};

bool operator==(const Callable& lhs, const Callable& rhs);
bool operator<(const Callable& lhs, const Callable& rhs);

//A call graph contains all possible calls
//Indirect calls (through mutable subroutine variables)
//are assumed to be able to reach any subroutine of
//the same type which appears in an RHS of assignment anywhere in whole
//program (including variable initializers)
struct CGNode
{
  set<Callable> out;
  Callable c;
};

struct CallGraph
{
  map<Callable, CGNode*> nodes;
};

extern CallGraph callGraph;

void buildCallGraph();

//internal

//find the global set of Callables which can be found in expressions,
//except those which are the callable of a CallExpr
//
//these may be the target of an indirect call somewhere
void determineIndirectReachable();

//(used by determineIndirectReachable)
//get the set of callable constants found in an expression which
//could eventually be assigned as a first-class function and called
set<Callable> getExpressionCallables(Expression* e)

#endif

