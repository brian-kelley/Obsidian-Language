#ifndef VARIABLE_H
#define VARIABLE_H

#include "Common.hpp"
#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "Scope.hpp"
#include "Expression.hpp"

struct Variable : public Node
{
  //ctor for global/static/member variables and arguments
  Variable(Scope* s, string name, Type* t, Expression* init, bool isStatic, bool compose = false);
  //ctor for local variables.
  //Locals declared in AST always belong to a block, but
  //locals created during optimization don't (null block, scope)
  Variable(string name, Type* t, Block* b = nullptr);
  bool isParameter();
  bool isLocal();
  bool isLocalOrParameter();
  bool isGlobal();
  bool isMember();
  //this resolve() just resolves type
  void resolveImpl();
  string name;
  Type* type;
  //the struct where this is a member, or NULL if global/local/param
  StructType* owner;
  Scope* scope;
  //the initial value of this variable/member, instead of the default "0"
  //for locals, this is left NULL since the initial assignment is a statement
  Expression* initial;
  //Each variable gets a unique ID when it is parsed.
  //This allows the IR to know the exact order in which globals are initialized.
  int id;
};

#endif

