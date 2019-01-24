#include "CSElim.hpp"

using namespace CSE;

bool CSElim::DefSet::insert(Variable* v, AssignIR* a)
{
  auto it = d.find(v);
  if(it == d.end())
  {
    d[v] = a;
    return true;
  }
  //Otherwise, replace existing (if different)
  if(a == d[v])
    return false;
  d[v] = a;
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
    d.erase(it);
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

bool operator==(const CSElim::DefSet& d1, const CSElim::DefSet& d2)
{
  return std::equal(d1.d.begin(), d1.d.end(), d2.d.begin(), d2.d.end());
}

void cse(SubroutineIR* subr)
{
  CSElim csElim(subr);
}

CSElim::CSElim(SubroutineIR* subr)
{
  int numBlocks = subr->blocks.size();
  vector<DefSet> definitions(numBlocks);
  //Go through each BB and record local definitions
  //(value of variables at block exit).
  //While going through the block, do local CSE and copy prop:
  //if X is read and there is a known definition of X, replace
  for(int i = 0; i < numBlocks; i++)
  {
    BasicBlock* block = subr->blocks[i];
    auto& defs = definitions[i];
    for(int j = block->start; j < block->end; j++)
    {
      //replace all non-write refs to vars whose definitions are vars
      if(auto assign = dynamic_cast<AssignIR*>(subr->stmts[j]))
      {
        //get the variable set that are written by the assign
        //(in the IR, there should always be exactly 1)
        auto writes = assign->dst->getWrites();
        INTERNAL_ASSERT(writes.size() == 1);
        Variable* written = *(writes.begin());
        //CSE only deals with local variables
        if(written->isLocal())
        {
          if(VarExpr* varDest = dynamic_cast<VarExpr*>(assign->dst))
          {
            defs[written] = assign->dst;
          }
          else
          {
            //some member or element of written is actually being changed,
            //forcing the invalidation of the entire previous definition
            auto it = defs.find(written);
            if(it != defs.end())
              defs.remove(it);
          }
        }
      }
    }
  }
  //Do dataflow analysis (whole CFG).
  //Initially, intersect all incoming definition sets and then
  //replace definitions that change in the block.
  //Iterate this until all DefSets stabilize.
}

bool CSElim::definitionsMatch(AssignIR* def1, AssignIR* def2)
{
  return *(def1->src) == *(def2->src);
}

bool CSElim::replaceExpr(Expression*& e, DefSet& defs)
{
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
  {
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

