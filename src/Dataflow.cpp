#include "Dataflow.hpp"
#include "Variable.hpp"
#include "Expression.hpp"
#include <algorithm>

using namespace IR;

namespace ReachingDefs
{
  vector<ReachingSet> compute(SubroutineIR* subr)
  {
    //Reaching defs for each block
    vector<ReachingSet> rd;
    auto numBlocks = subr->blocks.size();
    vector<bool> inQueue(numBlocks, true);
    queue<int> processQueue;
    for(size_t i = 0; i < subr->blocks.size(); i++)
      processQueue.push(i);
    while(processQueue.size())
    {
      int procInd = processQueue.front();
      processQueue.pop();
      BasicBlock* process = subr->blocks[procInd];
      inQueue[procInd] = false;
      ReachingSet& rs = rd[procInd];
      //save the previous def set
      auto old = rs;
      //clear the real one (recomputing from scratch)
      rs.clear();
      //union in all incoming
      for(auto pred : process->in)
        meet(rs, rd[pred->index]);
      //then apply each statements
      for(int i = process->start; i < process->end; i++)
      {
        transfer(rs, subr->stmts[i]);
      }
      //if any changes happened, must update successors
      if(old != rd[procInd])
      {
        for(auto succ : process->out)
        {
          auto succInd = succ->index;
          if(!inQueue[succInd])
          {
            inQueue[succInd] = true;
            processQueue.push(succInd);
          }
        }
      }
    }
    return rd;
  }

  void transfer(ReachingSet& r, StatementIR* stmt)
  {
    //Only assignments affect reaching defs. It's assumed that
    //calls with side effects can leave the variable unchanged, so the
    //definition reaches across in caller.
    //
    //Reaching defs are gen'd by assigns
    //and killed by assigns to the same variable.
    if(auto assign = dynamic_cast<AssignIR*>(stmt))
    {
      //TODO: support killing defs with other kinds of LHS:
      //struct members, array indices
      if(dynamic_cast<VarExpr*>(assign->dst))
      {
        for(auto it = r.begin(); it != r.end();)
        {
          if(*assign->dst == *(*it)->dst)
          {
            r.erase(it++);
            //there will be at most one definition active per variable
            break;
          }
          else
            it++;
        }
      }
      //no matter what LHS is, always add the new definition
      r.insert(assign);
    }
  }

  void meet(ReachingSet& into, ReachingSet& from)
  {
    //Meet is just union, very easy
    for(auto d : from)
      into.insert(d);
  }

  bool operator==(const ReachingSet& r1, const ReachingSet& r2)
  {
    if(r1.size() != r2.size())
      return false;
    for(auto d : r1)
    {
      if(r2.find(d) == r2.end())
        return false;
    }
    return false;
  }
}

namespace Liveness
{
  vector<LiveSet> compute(SubroutineIR* subr)
  {
    //Reaching defs for each block
    vector<LiveSet> rd;
    auto numBlocks = subr->blocks.size();
    vector<bool> inQueue(numBlocks, true);
    queue<int> processQueue;
    for(int i = subr->blocks.size() - 1; i >= 0; i--)
      processQueue.push(i);
    while(processQueue.size())
    {
      int procInd = processQueue.front();
      processQueue.pop();
      BasicBlock* process = subr->blocks[procInd];
      inQueue[procInd] = false;
      LiveSet& rs = rd[procInd];
      //save the previous def set
      auto old = rs;
      //clear the real one (recomputing from scratch)
      rs.clear();
      //union live sets at beginning of all successors
      for(auto succ : process->out)
        meet(rs, rd[succ->index]);
      //then apply each statements (in reverse)
      for(int i = process->end - 1; i >= process->start; i--)
      {
        transfer(rs, subr->stmts[i]);
      }
      //if any changes happened, must update predecessors 
      if(old != rd[procInd])
      {
        for(auto pred : process->in)
        {
          auto predInd = pred->index;
          if(!inQueue[predInd])
          {
            inQueue[predInd] = true;
            processQueue.push(predInd);
          }
        }
      }
    }
    return rd;
  }

  void transfer(LiveSet& r, StatementIR* stmt)
  {
    //get the input expressions
    set<Variable*> read;
    stmt->getReads(read);
    for(auto v : read)
    {
      r.insert(v);
    }
  }

  void meet(LiveSet& into, LiveSet& from)
  {
    //Meet is just union, very easy
    for(auto v : from)
      into.insert(v);
  }

  bool operator==(const LiveSet& l1, const LiveSet& l2)
  {
    if(l1.size() != l2.size())
      return false;
    for(auto v : l1)
    {
      if(l2.find(v) == l2.end())
        return false;
    }
    return false;
  }
}

