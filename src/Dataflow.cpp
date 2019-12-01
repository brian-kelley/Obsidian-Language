/*
#include "Dataflow.hpp"
#include "Variable.hpp"
#include "Expression.hpp"
#include <algorithm>

using namespace IR;

ReachingDefs::ReachingDefs(SubroutineIR* subr)
{
  int numBlocks = subr->blocks.size();
  //scan through all statements for assignments
  for(auto stmt : subr->stmts)
  {
    if(auto assign = dynamic_cast<AssignIR*>(stmt))
    {
      allAssigns.push_back(assign);
      assignTable[assign] = assignTable.size();
    }
  }
  int numAssigns = allAssigns.size();
  //all assignments initially non-reaching everywhere
  for(int i = 0; i < numBlocks; i++)
  {
    reaching.emplace_back(numAssigns, false);
  }
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
    ReachingSet& rs = reaching[procInd];
    //save the previous def set
    auto old = rs;
    //clear the real one (recomputing from scratch)
    for(int i = 0; i < numAssigns; i++)
      rs[i] = false;
    //union in all incoming
    for(auto pred : process->in)
      unionMeet(rs, reaching[pred->index]);
    //then apply each statements
    for(int i = process->start; i < process->end; i++)
    {
      transfer(rs, subr->stmts[i]);
    }
    //if any changes happened, must update successors
    if(old != reaching[procInd])
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
}

void ReachingDefs::transfer(ReachingSet& r, StatementIR* stmt)
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
    //struct members, array indices, globals
    //For now, just kill 
    if(dynamic_cast<VarExpr*>(assign->dst))
    {
      for(size_t i = 0; i < allAssigns.size(); i++)
      {
        if(r[i] && *assign->dst == *allAssigns[i]->dst)
          r[i] = false;
      }
    }
    //no matter what LHS is, always add the new definition
    r[assignTable[assign]] = true;
  }
}

Liveness::Liveness(SubroutineIR* subr)
{
  //collect a list of all local vars (including parameters)
  //DFS through scopes
  //Reaching defs for each block
  auto numBlocks = subr->blocks.size();
  for(auto v : subr->vars)
  {
    varTable[v] = allVars.size();
    allVars.push_back(v);
  }
  auto numVars = allVars.size();
  for(int i = 0; i < numBlocks; i++)
    live.emplace_back(numVars, false);
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
    LiveSet& rs = live[procInd];
    //save the previous def set
    LiveSet old = rs;
    //clear the real one (recomputing from scratch)
    for(int i = 0; i < numVars; i++)
      rs[i] = false;
    //union live sets at beginning of all successors
    for(auto succ : process->out)
      unionMeet(rs, live[succ->index]);
    //then apply each statements (in reverse)
    for(int i = process->end - 1; i >= process->start; i--)
    {
      transfer(rs, subr->stmts[i]);
    }
    //if any changes happened, must update predecessors 
    if(old != live[procInd])
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
}

bool Liveness::isLive(LiveSet& l, Variable* v)
{
  //variables not in table aren't local (assume always live)
  auto it = varTable.find(v);
  if(it == varTable.end())
    return true;
  return l[it->second];
}

void Liveness::transfer(LiveSet& r, StatementIR* stmt)
{
  //get the input expressions
  set<Variable*> read;
  stmt->getReads(read);
  for(auto v : read)
  {
    auto it = varTable.find(v);
    if(it != varTable.end())
    {
      r[it->second] = true;
    }
  }
}

void unionMeet(vector<bool>& into, vector<bool>& from)
{
  if(into.size() != from.size())
  {
    cout << "Trying to meet set of size " << from.size() << " into " << into.size() << '\n';
    INTERNAL_ERROR;
  }
  for(size_t i = 0; i < into.size(); i++)
  {
    into[i] = into[i] || from[i];
  }
}
*/
