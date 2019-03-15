#include "DeadCodeElim.hpp"
#include "Dataflow.hpp"
#include "Variable.hpp"
#include "CallGraph.hpp"

using namespace IR;

bool deadCodeElim(SubroutineIR* subr)
{
  //replace cond jumps with constant conditions with regular jumps,
  //and remove always-true and always-false assertions
  bool updatePhase1 = false;
  for(size_t i = 0; i < subr->stmts.size(); i++)
  {
    if(auto condJump = dynamic_cast<CondJump*>(subr->stmts[i]))
    {
      if(auto boolConst = dynamic_cast<BoolConstant*>(condJump->cond))
      {
        updatePhase1 = true;
        if(boolConst->value)
        {
          //branch never taken (does nothing)
          subr->stmts[i] = nop;
        }
        else
        {
          //always taken (just a regular jump)
          subr->stmts[i] = new Jump(condJump->taken);
        }
      }
    }
    else if(auto asi = dynamic_cast<AssertionIR*>(subr->stmts[i]))
    {
      if(auto constAsserted = dynamic_cast<BoolConstant*>(asi->asserted))
      {
        if(constAsserted->value)
        {
          //assert(true) is a no-op
          subr->stmts[i] = nop;
          updatePhase1 = true;
        }
        else
        {
          //assert(false) will always fail,
          //so tell user about it at compile time
          errMsgLoc(asi->asserted, "asserted condition always false");
        }
      }
    }
  }
  if(updatePhase1)
  {
    subr->buildCFG();
  }
  //do a breadth-first search of reachability from the first
  //BB to delete all unreachable code in one pass
  enum
  {
    NOT_VISITED,
    QUEUED,
    VISITED
  };
  map<BasicBlock*, char> blockVisits;
  for(auto bb : subr->blocks)
  {
    blockVisits[bb] = NOT_VISITED;
  }
  queue<BasicBlock*> visitQueue;
  visitQueue.push(subr->blocks[0]);
  blockVisits[subr->blocks[0]] = QUEUED;
  for(size_t i = 1; i < subr->blocks.size(); i++)
  {
    blockVisits[subr->blocks[i]] = NOT_VISITED;
  }
  while(visitQueue.size())
  {
    BasicBlock* process = visitQueue.front();
    visitQueue.pop();
    blockVisits[process] = VISITED;
    for(auto neighbor : process->out)
    {
      if(blockVisits[neighbor] == NOT_VISITED)
      {
        visitQueue.push(neighbor);
        blockVisits[neighbor] = QUEUED;
      }
    }
  }
  bool updatePhase2 = false;
  for(auto bb : subr->blocks)
  {
    if(blockVisits[bb] == NOT_VISITED)
    {
      updatePhase2 = true;
      for(int i = bb->start; i < bb->end; i++)
      {
        subr->stmts[i] = nop;
      }
    }
  }
  //then delete NOPs and rebuild CFG
  if(updatePhase2)
    subr->buildCFG();
  return updatePhase1 || updatePhase2;
}

void deadStoreElim(SubroutineIR* subr)
{
  ReachingDefs reaching(subr);
  int numAssigns = reaching.allAssigns.size();
  Liveness live(subr);
  int numVars = live.allVars.size();
  //Record which defs actually reach a usage (unused defs may be deleted)
  vector<bool> defsUsed(numAssigns, false);
  //Within each block, compute live set entry to each stmt (backwards)
  for(size_t b = 0; b < subr->blocks.size(); b++)
  {
    //compute the live and reaching sets at the beginning of each statement
    BasicBlock* block = subr->blocks[b];
    //First phase: delete assignments modifying dead variables.
    {
      LiveSet stmtLive(numVars, false);
      for(auto succ : block->out)
      {
        unionMeet(stmtLive, live.live[succ->index]);
      }
      for(int i = block->end - 1; i >= block->start; i--)
      {
        live.transfer(stmtLive, subr->stmts[i]);
        if(auto assign = dynamic_cast<AssignIR*>(subr->stmts[i]))
        {
          Variable* v = assign->dst->getWrite();
          if(!live.isLive(stmtLive, v))
          {
            if(assign->src->hasSideEffects())
              subr->stmts[i] = new EvalIR(assign->src);
            else
              subr->stmts[i] = nop;
          }
        }
      }
    }
    //Second phase: for every usage of V, flag defs of V reaching usage.
    {
      ReachingSet stmtReach(numAssigns, false);
      for(auto pred : block->in)
        unionMeet(stmtReach, reaching.reaching[pred->index]);
      for(int i = block->start; i < block->end; i++)
      {
        reaching.transfer(stmtReach, subr->stmts[i]);
        set<Variable*> used;
        subr->stmts[i]->getReads(used);
        for(int j = 0; j < numAssigns; j++)
        {
          //(optimization) test as few defs as possible
          if(!defsUsed[j] && stmtReach[j])
          {
            Variable* defWrite = reaching.allAssigns[j]->dst->getWrite();
            if(used.find(defWrite) != used.end())
              defsUsed[j] = true;
          }
        }
      }
    }
  }
  //Remove all unused assignments to locals.
  for(int i = 0; i < numAssigns; i++)
  {
    AssignIR* assign = reaching.allAssigns[i];
    if(!defsUsed[i] && assign->dst->getWrite()->isLocalOrParameter())
    {
      if(assign->src->hasSideEffects())
        subr->stmts[assign->intLabel] = new EvalIR(assign->src);
      else
        subr->stmts[assign->intLabel] = nop;
    }
  }
  //Delete completely unused variables (those which are not live at entry)
  for(int i = 0; i < numVars; i++)
  {
    //isLocal() is false for parameters
    if(!live.live[0][i] && live.allVars[i]->isLocal())
      subr->vars.erase(live.allVars[i]);
  }
  //Delete nops and rebuild CFG
  subr->buildCFG();
}

void unusedSubrElim()
{
  callGraph.rebuild();
  unordered_set<Callable> reachable;
  unordered_set<CallableType*> reachableIndirect;
  vector<Callable> visitStack;
  visitStack.push_back(Callable(mainSubr));
  while(visitStack.size())
  {
    Callable process = visitStack.back();
    visitStack.pop_back();
    auto& node = callGraph.nodes[process];
    for(Callable direct : node.outDirect)
    {
      if(reachable.find(direct) == reachable.end())
      {
        visitStack.push_back(direct);
        reachable.insert(direct);
      }
    }
    for(CallableType* indirect : node.outIndirect)
    {
      if(reachableIndirect.find(indirect) == reachableIndirect.end())
      {
        reachableIndirect.insert(indirect);
        //note: indirectReachables is computed with the CallGraph,
        //contains sets of possibly indirectly-called subroutines
        //for a given CallableType
        auto& r = indirectReachables[indirect];
        for(auto c : r)
        {
          if(reachable.find(c) == reachable.end())
          {
            visitStack.push_back(c);
            reachable.insert(c);
          }
        }
      }
    }
  }
  //finally, delete IR for all unreachable subroutines
  for(auto it = ir.begin(); it != ir.end();)
  {
    auto subr = it->first;
    if(reachable.find(Callable(subr)) == reachable.end())
    {
      cout << "Eliminated unreachable subroutine " << subr->name << '\n';
      ir.erase(it++);
    }
    else
      it++;
  }
  for(auto it = externIR.begin(); it != externIR.end();)
  {
    if(reachable.find(Callable(*it)) == reachable.end())
    {
      cout << "Eliminated unreachable external subroutine " << (*it)->name << '\n';
      externIR.erase(it++);
    }
    else
      it++;
  }
  //now, rebuild the call graph
  callGraph.rebuild();
}

void unusedGlobalElim()
{
  //Make a list of all global (static/module-scope) vars
  //then track their usage (read)
  unordered_set<Variable*> usedGlobals;
  for(auto& s : ir)
  {
    set<Variable*> locallyUsed;
    auto subrIR = s.second;
    for(auto stmt : subrIR->stmts)
    {
      stmt->getReads(locallyUsed);
    }
    for(auto used : locallyUsed)
    {
      if(used->isGlobal())
      {
        usedGlobals.insert(used);
      }
    }
  }
  //now, delete each assignment modifying an unused global
  //(only assignments matter for this)
  for(auto& s : ir)
  {
    auto subrIR = s.second;
    for(size_t i = 0; i < subrIR->stmts; i++)
    {
      if(auto assign = dynamic_cast<AssignIR*>(subr->stmts[i]))
      {
        auto wv = assign->dst->getWrite();
        if(wv->isGlobal() && usedGlobals.find(wv) == usedGlobals.end())
        {
          subrIR->stmts[i] = nop;
        }
      }
    }
  }
  //finally, walk the scope tree and delete unused global var decls
  Scope::walk([&] (Scope* s)
  {
    //globals can be declared in Module and StructType only
    if(s->node.is<Module*>() || s->node.is<StructType*>())
    {
      for(auto it = s->names.begin(); it != s->names.end();)
      {
        auto& name = it->second;
        bool removed = false;
        if(name.kind == Name::VARIABLE)
        {
          Variable* var = (Variable*) name.item;
          if(var->isGlobal() && usedGlobals.find(var) == usedGlobals.end())
          {
            s->names.erase(it++);
            removed = true;
          }
        }
        if(!removed)
          it++;
      }
    }
  });
}

