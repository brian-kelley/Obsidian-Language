#include "CSElim.hpp"

using namespace CSElim;
using namespace IR;

void cse(SubroutineIR* subr)
{
  auto numBlocks = subr->blocks.size();
  if(numBlocks == 0)
    return;
  vector<DefSet> definitions(numBlocks);
  //CSE iterates until the IR fully stabilizes
  //(all opportunities to eliminate computations are done)
  bool update = false;
  int passes = 0;
  do
  {
    for(auto& defSet : definitions)
    {
      defSet.clear();
    }
    update = false;
    //First phase: compute the set of definitely-available definitions
    //at the END of each BB
    //This process runs until all def sets stabilize.
    //Changes to one def set require recomputing all successors.
    //Can't do any replacements yet.
    queue<BasicBlock*> processQ;
    processQ.push(subr->blocks[0]);
    vector<bool> queued(numBlocks, false);
    queued[0] = true;
    while(processQ.size())
    {
      BasicBlock* process = processQ.front();
      processQ.pop();
      auto procInd = process->index;
      queued[procInd] = false;
      //Deep copy the def set (need to check for updates)
      //take defs from imm. dom, and intersect other preds
      auto& procDefs = definitions[procInd];
      DefSet oldDefs = procDefs;
      procDefs = meet(definitions, subr, process);
      //apply transfer function for each statement in process
      for(int i = process->start; i < process->end; i++)
      {
        //Exprs with side effects are only allowed as RHS of an assignment, and EvalIR
        if(auto assign = dynamic_cast<AssignIR*>(subr->stmts[i]))
        {
          transfer(assign, procDefs);
          if(assign->src->hasSideEffects())
            transferSideEffects(procDefs);
        }
        else if(auto eval = dynamic_cast<EvalIR*>(subr->stmts[i]))
        {
          //if eval has no side effects, might as well delete it now
          if(eval->eval->hasSideEffects())
            transferSideEffects(procDefs);
          else
            subr->stmts[i] = nop;
        }
      }
      if(oldDefs != procDefs)
      {
        //mark all successors for processing
        for(auto succ : process->out)
        {
          if(!queued[succ->index])
          {
            queued[succ->index] = true;
            processQ.push(succ);
          }
        }
      }
    }
    //now, have up-to-date avail sets
    //do CSE (sequentially per block)
    //after each statement, do the "transfer"
    for(auto& b : subr->blocks)
    {
      //need to operate on a copy of definitions, so that
      //original stays intact
      DefSet localDefs = meet(definitions, subr, b);
      for(int s = b->start; s < b->end; s++)
      {
        //transfers must be from the original set of expressions
        auto stmt = subr->stmts[s];
        if(auto assign = dynamic_cast<AssignIR*>(stmt))
        {
          //deep copy src
          Expression* newSrc = assign->src->copy();
          //modify the copy only
          update = replaceExpr(newSrc, localDefs) || update;
          //then apply transfer from original
          transfer(assign, localDefs);
          //now can change assign.
          assign->src = newSrc;
          //if assign is a no-op, delete it
          //can simply compare left and right
          if(*(assign->dst) == *(assign->src))
          {
            subr->stmts[s] = nop;
            update = true;
          }
          if(assign->src->hasSideEffects())
            transferSideEffects(localDefs);
        }
        else if(auto cj = dynamic_cast<CondJump*>(stmt))
          update = replaceExpr(cj->cond, localDefs) || update;
        else if(auto ev = dynamic_cast<EvalIR*>(stmt))
        {
          update = replaceExpr(ev->eval, localDefs) || update;
          if(ev->eval->hasSideEffects())
            transferSideEffects(localDefs);
        }
        else if(auto ret = dynamic_cast<ReturnIR*>(stmt))
        {
          if(ret->expr)
            update = replaceExpr(ret->expr, localDefs) || update;
        }
        else if(auto pr = dynamic_cast<PrintIR*>(stmt))
          update = replaceExpr(pr->expr, localDefs) || update;
        else if(auto as = dynamic_cast<AssertionIR*>(stmt))
          update = replaceExpr(as->asserted, localDefs) || update;
      }
    }
    passes++;
  }
  while(update);
  cout << "  Did CSE on " << subr->subr->name << " in " << passes << " passes.\n";
  if(update)
    subr->buildCFG();
}

namespace CSElim
{
  void DefSet::insert(Variable* v, AssignIR* a)
  {
    d[v] = a;
    //Insert the avail expression entry too, but don't overwrite if exists
    auto it = avail.find(a->src);
    if(it == avail.end())
      avail[a->src] = v;
  }

  void DefSet::intersect(Variable* v, AssignIR* a)
  {
    d.erase(v);
    avail.erase(a->src);
  }

  void DefSet::invalidate(Variable* v)
  {
    auto it = d.find(v);
    if(it != d.end())
    {
      Expression* def = it->second->src;
      d.erase(it);
      avail.erase(avail.find(def));
      //check if def is available from another variable
      for(auto& remain : d)
      {
        if(*remain.second->src == *def)
        {
          avail[def] = remain.first;
          break;
        }
      }
    }
  }

  bool DefSet::defined(Variable* v)
  {
    return d.find(v) != d.end();
  }

  void DefSet::clear()
  {
    d.clear();
    avail.clear();
  }

  Expression* DefSet::getDef(Variable* v)
  {
    auto it = d.find(v);
    if(it == d.end())
      return nullptr;
    return it->second->src;
  }

  Variable* DefSet::varForExpr(Expression* e)
  {
    auto it = avail.find(e);
    if(it == avail.end())
      return nullptr;
    return it->second;
  }

  bool replaceExpr(Expression*& e, DefSet& defs)
  {
    Variable* var = defs.varForExpr(e);
    if(var)
    {
      //e has already been computed, so replace it
      e = new VarExpr(var);
      e->resolve();
      return true;
    }
    bool update = false;
    //otherwise, try all subexpressions
    if(auto ua = dynamic_cast<UnaryArith*>(e))
    {
      update = replaceExpr(ua->expr, defs);
    }
    else if(auto ba = dynamic_cast<BinaryArith*>(e))
    {
      update = replaceExpr(ba->lhs, defs);
      update = replaceExpr(ba->rhs, defs) || update;
    }
    else if(auto cl = dynamic_cast<CompoundLiteral*>(e))
    {
      for(auto& m : cl->members)
      {
        update = replaceExpr(m, defs) || update;
      }
    }
    else if(auto ind = dynamic_cast<Indexed*>(e))
    {
      update = replaceExpr(ind->group, defs);
      update = replaceExpr(ind->index, defs) || update;
    }
    else if(auto al = dynamic_cast<ArrayLength*>(e))
    {
      update = replaceExpr(al->array, defs);
    }
    else if(auto as = dynamic_cast<AsExpr*>(e))
    {
      update = replaceExpr(as->base, defs);
    }
    else if(auto is = dynamic_cast<IsExpr*>(e))
    {
      update = replaceExpr(is->base, defs);
    }
    else if(auto call = dynamic_cast<CallExpr*>(e))
    {
      update = replaceExpr(call->callable, defs);
      for(auto& a : call->args)
      {
        update = replaceExpr(a, defs) || update;
      }
    }
    else if(auto conv = dynamic_cast<Converted*>(e))
    {
      update = replaceExpr(conv->value, defs);
    }
    return update;
  }

  void transfer(AssignIR* a, DefSet& defs)
  {
    //First, process kills: delete all definitions reading the var
    //modified in a
    //
    //(this includes reads in both lhs and rhs of def)
    auto writeSet = a->dst->getWrites();
    INTERNAL_ASSERT(writeSet.size() == 1);
    Variable* w = *(writeSet.begin());
    //for now, only keeping defs of locals
    vector<Variable*> killedDefs;
    for(auto& d : defs.d)
    {
      Expression* defRHS = d.second->src;
      auto rhsReads = defRHS->getReads();
      if(rhsReads.find(w) != rhsReads.end())
        killedDefs.push_back(d.first);
    }
    for(auto k : killedDefs)
    {
      defs.invalidate(k);
    }
    //And kill the old definition of w
    defs.invalidate(w);
    //If w is fully defined (it alone is the LHS), make a new definition
    //If w is partially assigned (only a member or index changing),
    //then can't store a definition
    if(dynamic_cast<VarExpr*>(a->dst))
    {
      defs.insert(w, a);
    }
  }

  void transferSideEffects(DefSet& defs)
  {
    //remove all definitions that read or write a global
    for(auto it = defs.d.begin(); it != defs.d.end();)
    {
      Expression* rhs = it->second->src;
      if(rhs->readsGlobals())
      {
        //need to delete this definition,
        //and corresponding expression-lookup entry (if any)
        defs.avail.erase(rhs);
        defs.d.erase(it++);
      }
      else
      {
        it++;
      }
    }
  }

  //Get incoming definition set for a given block
  DefSet meet(vector<DefSet>& definitions, SubroutineIR* subr, BasicBlock* b)
  {
    int immDom = -1;
    for(auto pred : b->in)
    {
      if(pred != b && b->dom[pred->index])
      {
        immDom = pred->index;
        break;
      }
    }
    DefSet& bDefs = definitions[b->index];
    //start by inserting immediate dominator's definitions (if there is one)
    if(immDom >= 0)
    {
      auto& domDefs = definitions[immDom].d;
      for(auto& d : domDefs)
      {
        bDefs.insert(d.first, d.second);
      }
    }
    //then intersect definitions of all predecessors into bDefs
    //if a pred has no definition for a variable, do nothing
    for(auto pred : b->in)
    {
      auto& predDefs = definitions[pred->index].d;
      for(auto& d : predDefs)
      {
        bDefs.intersect(d.first, d.second);
      }
    }
    return bDefs;
  }
}

bool operator==(const CSElim::DefSet& d1, const CSElim::DefSet& d2)
{
  if(d1.d.size() != d2.d.size())
    return false;
  if(d1.avail.size() != d2.avail.size())
    return false;
  for(auto& def : d1.d)
  {
    auto it2 = d2.d.find(def.first);
    if(it2 == d2.d.end() || def.second != it2->second)
      return false;
  }
  //don't need to compare avail - it's just an inverse mapping
  return true;
}

bool operator!=(const CSElim::DefSet& d1, const CSElim::DefSet& d2)
{
  return !(d1 == d2);
}

