#ifndef VARIABLE_H
#define VARIABLE_H

#include "Misc.hpp"
#include "Utils.hpp"
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
  Variable(Scope* s, Parser::VarDecl* ast);
  Scope* scope;
  string name;
  TypeSystem::Type* type;
  bool isStatic;
};

#endif

