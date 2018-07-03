#ifndef CFG_H
#define CFG_H

#include "IR.hpp"
#include "Common.hpp"

//Basic blocks start at subr entry or branch targets,
//and end just after jumps or returns
struct BasicBlock
{
  void link(int next);
  vector<int> in;
  vector<int> out;
  int start;
  int end;
};

//Per-subroutine control flow graph
//Edges are statements, 
struct CFG
{
  CFG(SubroutineIR* s);
  vector<BasicBlock> blocks;
  SubroutineIR* subr;
};

#endif

