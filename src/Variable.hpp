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

struct Variable : public Node
{
  //ctor for global/static/member variables and arguments
  Variable(Scope* s, string name, TypeSystem::Type* t, Expression* init, bool isStatic, bool compose = false);
  //ctor for local variables
  Variable(string name, TypeSystem::Type* t, Block* b);
  //this resolve() just resolves type
  void resolve(bool final);
  string name;
  TypeSystem::Type* type;
  //the struct where this is a member, or NULL if static/local
  StructType* owner;
  Scope* scope;
  //the initial value of this variable/member, instead of the default "0"
  //for locals, this is left NULL since the initial assignment is a statement
  Expression* initial;
  //for local variables only: the position of VarDecl in the list of statements
  //used to check for use-before-declare errors
  int blockPos;
};

#endif

