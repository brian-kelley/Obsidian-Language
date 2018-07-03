#include "IRDebug.hpp"

using namespace IR;

void IRDebug::dumpIR(string filename)
{
  //for each subroutine, print basic blocks to string and
  //then add CFG edges

  //each subroutine gets its own graph in the file
  ofstream outputFile(filename);
  //use a sequential counter to disambiguate
  //subroutines with same name
  int counter = 0;
  for(auto& subrPair : IR::ir)
  {
    SubroutineIR* subr = subrPair.second;
    Oss nameOss;
    nameOss << subrPair.first->name << " (" << counter++ << ")";
    Dotfile dotGraph(nameOss.str());
    map<BasicBlock*, int> nodes;
    for(auto bb : subr->blocks)
    {
      Oss bbStream;
      for(int stmt = bb->start; stmt < bb->end; stmt++)
      {
        bbStream << subr->stmts[stmt];
      }
      nodes[bb] = dotGraph.createNode(bbStream.str());
    }
    for(auto bb : subr->blocks)
    {
      for(auto outgoing : bb->out)
      {
        dotGraph.createEdge(nodes[bb], nodes[outgoing]);
      }
    }
    dotGraph.write(outputFile);
  }
  outputFile.close();
}

