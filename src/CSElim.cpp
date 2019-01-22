#include "CSElim.hpp"

using namespace CSE;

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

void CSElim::copyProp(Expression*& e, DefSet& defs)
{
  if(auto var = dynamic_cast<VarExpr*>(e))
  {
    auto it = defs.find(var);
    if(it != defs.end())
    {
    }
  }
  else if(auto ba = dynamic_cast<BinaryArith*>(e))
  {
  }
}

bool CSElim::elimComputation(AssignIR* a, DefSet& defs)
{
  //if a's RHS is nontrivial AND has no side effects,
  //try to find a variable defined with the same value
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
  for(auto& def : defs)
  {
  }
}

//Intersect all definition sets of bb's predecessors
//Definitions must match exactly to be kept
DefSet CSElim::meet(SubroutineIR* subr, int bb)
{
  DefSet d;
  auto& defs = d.defs;
  auto& blacklist = d.blacklist;
  //Every node can have at most one immediate dominator (if there were > 1,
  //each would represent a different path into the node so they don't dominate)
  //
  //So, start with the imm. dominator's def set if there is one.
  //
  //Then intersect defs with those of all incoming blocks (including self)
  queue<BasicBlock*> processQ;
  process.push(blocks[0]);
  vector<bool> queued(blocks.size(), false);
  queued[0] = true;
  vector<int> intersectCounting(blocks.size());
  while(processQ.size())
  {
  }
  return d;
}

