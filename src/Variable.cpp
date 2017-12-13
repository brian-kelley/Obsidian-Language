#include "Variable.hpp"

Variable::Variable(Scope* s, Parser::VarDecl* ast)
{
  name = ast->name;
  //find type using deferred lookup
  TypeSystem::TypeLookup tl(ast->type, s);
  TypeSystem::typeLookup->lookup(tl, type);
}

Variable::Variable(Scope* s, string n, Parser::TypeNT* t)
{
  name = n;
  TypeSystem::TypeLookup tl(t, s);
  TypeSystem::typeLookup->lookup(tl, type);
}

Variable::Variable(Scope* s, string n, TypeSystem::Type* t)
{
  this->name = n;
  this->type = t;
}

