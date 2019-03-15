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
  Dotfile dotGraph("IR");
  for(auto& subrIR : ir)
  {
    map<BasicBlock*, int> nodes;
    for(auto bb : subrIR->blocks)
    {
      Oss bbStream;
      if(bb == subr->blocks.front())
        bbStream << "// Subroutine " << subrIR->subr->name << "\\n";
      bbStream << "// Basic Block " << bb->start << ":" << bb->end << "\\n";
      for(int stmt = bb->start; stmt < bb->end; stmt++)
      {
        bbStream << subrIR->stmts[stmt] << "\\n";
      }
      nodes[bb] = dotGraph.createNode(bbStream.str());
    }
    for(auto bb : subrIR->blocks)
    {
      for(auto outgoing : bb->out)
      {
        dotGraph.createEdge(nodes[bb], nodes[outgoing]);
      }
    }
  }
  dotGraph.write(outputFile);
  outputFile.close();
}

