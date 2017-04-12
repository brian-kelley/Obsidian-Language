#ifndef SCOPE_H
#define SCOPE_H

#include <iostream>
#include <string>
#include <vector>

#include "AutoPtr.hpp"
#include "TypeSystem.hpp"

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
  string getFullPath();                 //get full, unambiguous name of scope (for C type names)
  Type* typeFromName(string name);
  Scope* parent;                        //parent of scope, or NULL for 
  vector<AP(Scope)> children;           //owned scopes
  vector<AP(Type)> types;               //types declared here
  vector<AP(Variable)> vars;            //variables declared here
  //funcs and procs are all declarations and/or definitions in scope
  //definition can go in a parent scope, unless this is a Block
  vector<AP(FuncPrototype)> funcs;
  vector<AP(ProcPrototype)> procs;
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

#endif

