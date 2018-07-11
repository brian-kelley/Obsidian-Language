#include "Dataflow.hpp"
#include "Variable.hpp"
#include "Expression.hpp"
#include <algorithm>

using namespace IR;

namespace Liveness
{
  map<SubroutineIR*, LiveSet*> liveSets;

  LiveSet::LiveSet(LiveSet* fw, LiveSet* bw)
  {
    for(auto bbLive : fw->live)
    {
      insertVars(bbLive.first, bbLive.second);
    }
    for(auto bbLive : fw->live)
    {
      insertVars(bbLive.first, bbLive.second);
    }
  }

  void LiveSet::insertVars(IR::BasicBlock* bb, set<Variable*> vars)
  {
    if(live.find(bb) == live.end())
    {
      live[bb] = vars;
    }
    else
    {
      live[bb].insert(vars.begin(), vars.end());
    }
  }

  void LiveSet::intersectVars(IR::BasicBlock* bb, set<Variable*> vars)
  {
    auto& thisSet = live[bb];
    for(auto var : vars)
    {
      if(thisSet.find(var) == thisSet.end())
        thisSet.erase(var);
    }
  }

  void buildAllLivesets()
  {
    for(auto s : IR::ir)
    {
      auto subr = s.second;
      LiveSet* forward = new LiveSet;
      LiveSet* backward = new LiveSet;
      for(int which = 0; which < 2; which++)
      {
        queue<BasicBlock*> toProcess;
        bool forwardDir = which == 0;
        LiveSet* current = forwardDir ? forward : backward;
        //have to visit every BB at least once,
        //then visit predecessors again after processing
        for(auto bb : subr->blocks)
        {
          current->live[bb] = subr->getReads(bb);
        }
        for(int i = subr->blocks.size() - 1; i >= 0; i--)
        {
          toProcess.push(subr->blocks[i]);
        }
        while(!toProcess.empty())
        {
          BasicBlock* process = toProcess.front();
          toProcess.pop();
          size_t oldSize = current->live.size();
          //here "incoming" means predecessor in order of current dataflow direction
          vector<BasicBlock*>* incoming = forwardDir ? &process->in : &process->out;
          vector<BasicBlock*>* outgoing = forwardDir ? &process->out : &process->in;
          for(auto predecessor : *incoming)
          {
            current->insertVars(process, current->live[predecessor]);
          }
          if(current->live.size() != oldSize)
          {
            //made an update, so need to visit all predecessors again
            for(auto successor : *outgoing)
            {
              toProcess.push(successor);
            }
          }
        }
      }
      for(auto bb : subr->blocks)
        forward->intersectVars(bb, backward->live[bb]);
      liveSets[subr] = forward;
      cout << "Variable liveness of subroutine " << s.first->name << ":\n";
      for(auto bb : subr->blocks)
      {
        cout << "  BB " << bb->start << ':' << bb->end << '\n';
        for(auto l : forward->live[bb])
        {
          cout << "    " << l->name << '\n';
        }
      }
      cout << '\n';
    }
  }
}

