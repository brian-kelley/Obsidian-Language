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
  Variable(Scope* s, Parser::VarDecl* ast);
  Variable(Scope* s, string name, Parser::TypeNT* t);
  Variable(Scope* s, string n, Parser::TypeNT* t);
  //Constructor for creating local variable with given name and type
  Variable(Scope* s, string name, TypeSystem::Type* t);
  string name;
  TypeSystem::Type* type;
};

#endif

