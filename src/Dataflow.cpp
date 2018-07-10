#include "Dataflow.hpp"
#include "Variable.hpp"
#include "Expression.hpp"

using namespace IR;

namespace Liveness
{
  map<SubroutineIR*, LiveSet*> liveSets;

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

  void buildAllLivesets()
  {
    for(auto s : IR::ir)
    {
      auto subr = s.second;
      LiveSet* live = new LiveSet;
      liveSets[subr] = live;
      queue<BasicBlock*> toProcess;
      //have to visit every BB at least once,
      //then visit predecessors again after processing
      for(auto bb : subr->blocks)
      {
        live->live[bb] = subr->getReads(bb);
      }
      for(int i = subr->blocks.size() - 1; i >= 0; i--)
      {
        toProcess.push(subr->blocks[i]);
      }
      while(!toProcess.empty())
      {
        BasicBlock* process = toProcess.front();
        toProcess.pop();
        //union all variables read in process
        size_t oldSize = live->live.size();
        for(auto successor : process->out)
        {
          live->insertVars(process, live->live[successor]);
        }
        if(live->live.size() != oldSize)
        {
          //made an update, so need to visit all predecessors again
          for(auto incoming : process->in)
          {
            toProcess.push(incoming);
          }
        }
      }
      cout << "Variable liveness of subroutine " << s.first->name << ":\n";
      for(auto bb : subr->blocks)
      {
        cout << "  BB " << bb->start << ':' << bb->end << '\n';
        for(auto l : live->live[bb])
        {
          cout << "    " << l->name << '\n';
        }
      }
      cout << '\n';
    }
  }
}

