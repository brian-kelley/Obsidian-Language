#include "CFG.hpp"
#include "Common.hpp"
#include "Scope.hpp"
#include "Subroutine.hpp"

CFG::CFG(Subroutine* s)
{
  //first, just partition all the statements
  //into basic blocks
  //(don't add edges yet)
  Block* b = s->body;
}

void CFG::BasicBlock::addStatement(Statement* s)
{
}

void CFG::BasicBlock::link(BasicBlock* next)
{
}

void CFG::buildBasicBlocks(Statement* s)
{
  if(Block* b = dynamic_cast<Block*>(s))
  {
  }
}

void CFG::linkBasicBlocks(Statement* s);
{
}

