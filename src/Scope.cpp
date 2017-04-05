#include "Scope.hpp"

/*******************************
*   Scope & subclasses impl    *
*******************************/

int BlockScope::nextBlockIndex = 0;

string Scope::getFullPath()
{
  if(parent)
    return parent->getFullPath + '_' + getLocalName()
  else
    return getLocalName();
}

/* ModuleScope */

ModuleScope::ModuleScope(Scope* parent)
{
  this->parent = parent;
}

ScopeType ModuleScope::getType()
{
  return ScopeType::MODULE;
}

string ModuleScope::getLocalName()
{
  return name;
}

/* StructScope */

ScopeType StructScope::getType()
{
  return ScopeType::STRUCT;
}

string StructScope::getLocalName()
{
  return name;
}

/* BlockScope */

ScopeType BlockScope::getType()
{
  return ScopeType::BLOCK;
}

string BlockScope::getLocalName()
{
  return string("B") + to_string(index);
}

