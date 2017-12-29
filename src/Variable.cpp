#include "Variable.hpp"

Variable::Variable(Scope* s, Parser::VarDecl* ast, bool member)
{
  scope = s;
  name = ast->name;
  isMember = member;
  //find type using deferred lookup
  TypeSystem::TypeLookup tl(ast->type, s);
  TypeSystem::typeLookup->lookup(tl, type);
  check();
}

Variable::Variable(Scope* s, string n, Parser::TypeNT* t, bool member)
{
  scope = s;
  name = n;
  isMember = member;
  TypeSystem::TypeLookup tl(t, s);
  TypeSystem::typeLookup->lookup(tl, type);
  check();
}

Variable::Variable(Scope* s, string n, TypeSystem::Type* t, bool member)
{
  scope = s;
  isMember = member;
  this->name = n;
  this->type = t;
  check();
}

void Variable::check()
{
  if(type == TypeSystem::primitives[Parser::TypeNT::VOID])
  {
    ERR_MSG("variable " << name << " declared void");
  }
}

