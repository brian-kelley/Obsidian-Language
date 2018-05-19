#include "Variable.hpp"

Variable::Variable(Scope* s, string n, TypeSystem::Type* t, bool isStatic, bool compose)
{
  scope = s;
  name = n;
  type = t;
  owner = s->getMemberContext();
  blockPos = -1;
  //if this variable is nonstatic and is inside a struct, add it as member
  if(!isStatic && owner)
  {
    //add this as a member of the struct
    owner->members.push_back(this);
    owner->composed.push_back(compose);
  }
}

Variable::Variable(string name, TypeSystem::Type* t, Block* b)
{
  scope = b->scope;
  name = n;
  type = t;
  owner = nullptr;
  blockPos = b->statementCount;
}

void Variable::resolve(bool final)
{
  resolveType(type, final);
  if(type->resolved)
  {
    resolved = true;
  }
}

