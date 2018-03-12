#ifndef VARIABLE_H
#define VARIABLE_H

#include "Common.hpp"
#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "Scope.hpp"
#include "Expression.hpp"

struct Scope;

namespace TypeSystem
{
  struct Type;
}

struct Variable
{
  //General constructor for static or local variables created through VarDecl
  Variable(Scope* s, Parser::VarDecl* ast, bool member = false);
  Variable(Scope* s, string name, Parser::TypeNT* t, bool member = false);
  Variable(Scope* s, string name, TypeSystem::Type* t, bool member = false);
  string name;
  TypeSystem::Type* type;
  //the struct where this is a member, or NULL if static/local
  Struct* owner;
  Scope* scope; //variables need to know their own scope for fn purity checking
};

#endif

