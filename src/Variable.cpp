#include "Variable.hpp"

Variable::Variable(Scope* s, string n, TypeSystem::Type* t, bool isStatic)
{
  scope = s;
  name = n;
  type = t;
  owner = nullptr;
  //blockPos = -1;
  for(Scope* iter = s; iter; iter = iter->parent)
  {
    if(iter->node.is<Block*>())
    {
      break;
    }
    else if(iter->node.is<StructType*>())
    {
      if(!isStatic)
        owner = iter->node.get<StructType*>();
      break;
    }
  }
}

Variable::Variable(string name, TypeSystem::Type* t, Block* b)
{
  scope = b->scope;
  name = n;
  type = t;
  owner = nullptr;
  //blockPos = b->statementCount;
}

void Variable::resolve(bool final)
{
  resolveType(type, final);
  if(type->resolved)
  {
    resolved = true;
  }
}

