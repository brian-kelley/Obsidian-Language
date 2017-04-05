#ifndef SCOPE_H
#define SCOPE_H

#include <iostream>
#include <string>
#include <vector>

#include "AutoPtr.hpp"
#include "TypeSystem.hpp"

struct Scope;

struct FuncPrototype
{
  FuncPrototype(Parser::FuncType& ft);
  Type* retType;
  vector<Type*> argTypes;
};

struct ProcPrototype
{
  ProcPrototype(Parser::ProcType& pt);
  bool nonterm;
  Type* retType;
  vector<Type*> argTypes;
};

struct Trait
{
  Scope* name;
  string name;
  vector<FuncPrototype*> funcs;
  vector<ProcPrototype*> procs;
};

struct Variable
{
  Scope* owner;
  string name;
  Type* type;
};

enum struct ScopeType
{
  MODULE,
  STRUCT,
  BLOCK   //includes func/proc bodies
};

//Scopes own all funcs/structs/traits/etc
struct Scope
{
  virtual ScopeType getType() = 0;
  virtual string getLocalName() = 0;
  string getFullPath();
  Scope* parent;
  vector<AP(Scope)> children;
  vector<AP(Variable)> vars;
  vector<AP(FuncPrototype)> funcs;
  vector<AP(ProcPrototype)> funcs;
  vector<AP(Trait)> traits;
};

struct ModuleScope : public Scope
{
  ModuleScope(Scope* parent);
  ScopeType getType();
  string getLocalName();
  string name;  //local name
};

struct StructScope : public Scope
{
  StructScope(Scope* parent);
  ScopeType getType();
  string getLocalName();
  string name;  //local name
};

struct BlockScope : public Scope
{
  BlockScope(Scope* parent);
  ScopeType getType();
  string getLocalName();
  int index;
  static int nextBlockIndex;
};

/*****************************************/
/* Scope hierarchy + type system loading */
/*****************************************/



#endif

