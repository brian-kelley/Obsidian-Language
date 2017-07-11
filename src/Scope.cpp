#include "Scope.hpp"

int BlockScope::nextBlockIndex = 0;

/*******************************
*   Scope & subclasses impl    *
*******************************/

Scope::Scope(Scope* parent)
{
  this->parent = parent;
  if(parent)
  {
    parent->children.push_back(this);
  }
}

string Scope::getFullPath()
{
  if(parent)
    return parent->getFullPath() + '_' + getLocalName();
  else
    return getLocalName();
}

/* ModuleScope */

ModuleScope::ModuleScope(string nameIn, Scope* parent, Parser::Module* astIn) : Scope(parent)
{
  name = nameIn;
  ast = astIn;
}

string ModuleScope::getLocalName()
{
  return name;
}

/* StructScope */

StructScope::StructScope(string nameIn, Scope* parent, Parser::StructDecl* astIn) : Scope(parent), ast(astIn), name(nameIn) {}

string StructScope::getLocalName()
{
  return name;
}

/* BlockScope */

BlockScope::BlockScope(Scope* parent, Parser::Block* astIn) : Scope(parent), ast(astIn), index(nextBlockIndex++) {}

string BlockScope::getLocalName()
{
  //TODO: prevent all other identifiers from having a name which could be confused as a block name
  return string("_B") + to_string(index);
}

