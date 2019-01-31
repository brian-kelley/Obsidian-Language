#include "CSElim.hpp"

using namespace CSE;

void CSElim::DefSet::insert(Variable* v, AssignIR* a)
{
  d[v] = a;
  //Insert the avail expression entry too, but don't overwrite
  auto it = avail.find(a->src);
  if(it == avail.end())
    avail[a->src] = v;
}

bool CSElim::DefSet::intersect(Variable* v, AssignIR* a)
{
  auto it = d.find(v);
  if(it != d.end() &&
      *a->src, *it->src)
  {
    //definitions for v differ; remove existing.
    d.erase(it);
  }
}

bool CSElim::DefSet::invalidate(Variable* v)
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

bool CSElim::DefSet::defined(Variable* v)
{
  return d.find(v) != d.end();
}

void CSElim::DefSet::clear()
{
  d.clear();
  avail.clear();
}

Expression* CSElim::DefSet::getDef(Variable* v)
{
  auto it = d.find(v);
  if(it == d.end())
    return nullptr;
  return it->second->src;
}

Variable* CSElim::DefSet::varForExpr(Expression* e)
{
  auto it = avail.find(e);
  if(it == avail.end())
    return nullptr;
  return it->second;
}

bool CSElim::operator==(const CSElim::DefSet& d1, const CSElim::DefSet& d2)
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
  //don't need to check avail - it's just an inverse mapping
  return true;
}

void cse(SubroutineIR* subr)
{
  CSElim csElim(subr);
}

CSElim::CSElim(SubroutineIR* subr)
  : definitions(subr->blocks.size())
{
  if(blocks.size() == 0)
    return;
  //CSE iterates until the IR fully stabilizes
  //(all opportunities to eliminate computations are done)
  bool generalUpdate = false;
  int passes = 0;
  do
  {
    for(auto& defSet : definitions)
    {
      defSet.clear();
    }
    generalUpdate = false;
    //First phase: compute the set of definitely-available definitions
    //at the END of each BB
    //This process runs until all def sets stabilize.
    //Changes to one def set require recomputing all successors.
    //Can't do any replacements yet.
    queue<BasicBlock*> processQ;
    process.push(blocks[0]);
    vector<bool> queued(blocks.size(), false);
    queued[0] = true;
    while(processQ.size())
    {
      BasicBlock* process = processQ.front();
      processQ.pop();
      auto procInd = process->index;
      queued[procInd] = false;
      auto& procDefs = definitions[procInd];
      //Deep copy the def set (need to check for updates)
      DefSet oldDefs = procDefs;
      procDefs.clear();
      //take defs from imm. dom, and intersect other preds
      meet(subr, process);
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
            subr->stmts[i] = Nop::nop;
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
    passes++;
  }
  while(generalUpdate);
  cout << "  Did CSE on " << subr->name << " in " << passes << " passes.\n";
  int numBlocks = subr->blocks.size();
}

bool CSElim::replaceExpr(Expression*& e, DefSet& defs)
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
    update = replaceExpr(ua->expr);
  }
  else if(auto ba = dynamic_cast<BinaryArith*>(e))
  {
    update = replaceExpr(ba->lhs);
    update = replaceExpr(ba->rhs) || update;
  }
  else if(auto cl = dynamic_cast<CompoundLiteral*>(e))
  {
    for(auto& m : cl->members)
    {
      update = replaceExpr(m) || update;
    }
  }
  else if(auto ind = dynamic_cast<Indexed*>(e))
  {
    update = replaceExpr(ind->group);
    update = replaceExpr(ind->index) || update;
  }
  else if(auto al = dynamic_cast<ArrayLength*>(e))
  {
    update = replaceExpr(al->array);
  }
  else if(auto as = dynamic_cast<AsExpr*>(e))
  {
    update = replaceExpr(as->base);
  }
  else if(auto is = dynamic_cast<IsExpr*>(e))
  {
    update = replaceExpr(is->base);
  }
  else if(auto call = dynamic_cast<CallExpr*>(e))
  {
    update = replaceExpr(call->callable);
    for(auto& a : call->args)
    {
      update = replaceExpr(a) || update;
    }
  }
  else if(auto conv = dynamic_cast<Converted*>(e))
  {
    update = replaceExpr(conv->value);
  }
  return update;
}

void CSElim::transfer(AssignIR* a, DefSet& defs)
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
  for(auto& d : defs)
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
    defs.insert(w, a->src);
  }
  return update;
}

bool CSElim::transferSideEffects(DefSet& defs)
{
  //remove all definitions that read or write a global
  for(auto it = defs.d.begin(); it != defs.d.end();)
  {
    Expression* rhs = it->second->src;
    if(rhs->readsGlobals())
    {
      //need to delete this definition,
      //and corresponding expression-lookup entry (if any)
      if(avail.find(rhs))
        avail.erase(rhs);
      defs.d.erase(it++);
    }
    else
    {
      it++;
    }
  }
}

//Get incoming definition set for a given block
void CSElim::meet(SubroutineIR* subr, BasicBlock* b)
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
}

