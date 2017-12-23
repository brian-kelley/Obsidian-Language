#include "Variable.hpp"

Variable::Variable(Scope* s, Parser::VarDecl* ast, bool member)
{
  scope = s;
  name = ast->name;
  isMember = member;
  //find type using deferred lookup
  TypeSystem::TypeLookup tl(ast->type, s);
  TypeSystem::typeLookup->lookup(tl, type);
}

Variable::Variable(Scope* s, string n, Parser::TypeNT* t, bool member)
{
  scope = s;
  name = n;
  isMember = member;
  TypeSystem::TypeLookup tl(t, s);
  TypeSystem::typeLookup->lookup(tl, type);
}

Variable::Variable(Scope* s, string n, TypeSystem::Type* t, bool member)
{
  scope = s;
  isMember = member;
  this->name = n;
  this->type = t;
}

