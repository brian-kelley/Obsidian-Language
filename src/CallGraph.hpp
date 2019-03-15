#ifndef CALL_GRAPH_H
#define CALL_GRAPH_H

#include "Common.hpp"
#include "IR.hpp"

struct Callable
{
  Callable()
  {
    s = nullptr;
  }
  Callable(IR::SubroutineIR* su) : s(su) {}
  Callable(ExternalSubroutine* es) : s(es) {}
  Callable(const Callable& other) : s(other.s) {}
  CallableType* getType() const
  {
    auto subrIR = dynamic_cast<IR::SubroutineIR*>(s);
    if(subrIR)
      return subrIR->subr->type;
    return dynamic_cast<ExternalSubroutine*>(s)->type;
  }
  //Dynamic cast to either Subroutine or ExternalSubroutine
  Node* s;
};

namespace std
{
  template<>
  struct hash<Callable>
  {
    size_t operator()(const Callable& c) const
    {
      return fnv1a(c.s);
    }
  };
}

bool operator==(const Callable& lhs, const Callable& rhs);
bool operator<(const Callable& lhs, const Callable& rhs);

//A call graph contains all possible calls
//Indirect calls (through mutable subroutine variables)
//are assumed to be able to reach any subroutine of
//the same type which appears in an RHS of assignment anywhere in whole
//program (including variable initializers)
struct CGNode
{
  set<Callable> outDirect;
  set<CallableType*> outIndirect;
};

struct CallGraph
{
  void rebuild();
  map<Callable, CGNode> nodes;
  void addNode(Callable c);
  void addEdge(IR::SubroutineIR* s, CallableType* indirect);
  void addEdge(IR::SubroutineIR* s, Callable direct);
  void addEdge(ExternalSubroutine* s, CallableType* indirect);
  void dump(string path);
};

extern CallGraph callGraph;
extern unordered_map<CallableType*, set<Callable>,
  TypeHash, TypeEqual> indirectReachables;

//(Internal - never call from outside)

//Find the global set of Callables which can be found in expressions
//BESIDES CallExprs. These are all assumed to be possible values of
//any callable expr of their type.
void determineIndirectReachable();

//Get the set of callable constants found in an expression.
//They are stored in an internal table.
void gatherIndirectCallables(Expression* e);
//Add a callable to the internal table
//(can be looked up by CallableType)
void registerIndirectCallable(Callable c);

#endif

