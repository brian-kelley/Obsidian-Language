#include "CSElim.hpp"

using namespace CSE;

bool CSElim::DefSet::insert(Variable* v, AssignIR* a)
{
  auto it = d.find(v);
  if(it != d.end() && *it == a)
    return false;
  d[v] = a;
  {
    //Don't overwrite "avail" variables -
    //older definitions are better for CSE
    //(stabilizes in fewer iterations)
    auto it2 = avail.find(a->src);
    if(it2 == avail.end())
      avail[a->src] = v;
  }
  return true;
}

bool CSElim::DefSet::intersect(Variable* v, AssignIR* a)
{
  auto it = d.find(v);
  if(it != d.end() &&
      !definitionsMatch(a, *it))
  {
    //definitions for v differ; remove existing.
    d.erase(it);
    return true;
  }
  //else: either definition matches existing, or there is no
  //existing def (no change)
  return false;
}

bool CSElim::DefSet::invalidate(Variable* v)
{
  auto it = d.find(v);
  if(it != d.end())
  {
    Expression* def = it->second->src;
    d.erase(it);
    avail.erase(avail.find(def));
    return true;
  }
  return false;
}

bool CSElim::DefSet::defined(Variable* v)
{
  return d.find(v) != d.end();
}

Expression* CSElim::DefSet::getDef(Variable* v)
{
  return d[v]->src;
}

Variable* CSElim::DefSet::varForExpr(Expression* e)
{
  auto it = avail.find(e);
  if(it != avail.end())
  {
    return it->second;
  }
  return nullptr;
}

bool CSElim::operator==(const CSElim::DefSet& d1, const CSElim::DefSet& d2)
{
  return std::equal(d1.d.begin(), d1.d.end(), d2.d.begin(), d2.d.end());
}

void cse(SubroutineIR* subr)
{
  CSElim csElim(subr);
}

CSElim::CSElim(SubroutineIR* subr)
{
  if(blocks.size() == 0)
    return;
  int numBlocks = subr->blocks.size();
  vector<DefSet> definitions(numBlocks);
  queue<BasicBlock*> processQ;
  process.push(blocks[0]);
  vector<bool> queued(blocks.size(), false);
  queued[0] = true;
  vector<int> intersectCounting(blocks.size());
  while(processQ.size())
  {
    BasicBlock* process = processQ.front();
    processQ.pop();
    auto procInd = process->index;
    queued[procInd] = false;
  }
}

bool CSElim::definitionsMatch(AssignIR* def1, AssignIR* def2)
{
  return *(def1->src) == *(def2->src);
}

bool CSElim::replaceExpr(Expression*& e, DefSet& defs)
{
  Variable* var = defs.varForExpr(e);
  if(var)
  {
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
  if(!w->isLocal())
    return;
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
    defs.erase(defs.find(k));
  }
  //Always kill the old definition of w
  {
    auto it = defs.find(w);
    if(it != defs.end())
      defs.erase(it);
  }
  //If w is fully defined (it alone is the LHS), make a new definition
  //If w is partially assigned (a member or index) then can't do this
  if(dynamic_cast<VarExpr*>(a->dst))
  {
    defs[w] = a->src;
  }
}

void CSElim::transferSideEffects(DefSet& defs)
{
}

//Get incoming definition set for a given block
void CSElim::meet(SubroutineIR* subr, BasicBlock* b)
{
  bool update = false;
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
  bDefs.d.clear();
  //start with immediate dominator's definitions (if there is one)
  if(immDom >= 0)
  {
    bDefs.d = definitions[immDom].d;
  }
  //intersect definitions of all predecessors into bDefs
  //if a pred has no definition, this does nothing
  for(auto pred : b->in)
  {
    for(auto& def : definitions[pred->index].d)
    {
      bDefs.intersect(def.first, def.second);
    }
  }
  return bDefs;
}

