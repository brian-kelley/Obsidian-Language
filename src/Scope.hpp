#ifndef SCOPE_H
#define SCOPE_H

#include <iostream>
#include <string>
#include <vector>

#include "AutoPtr.hpp"
#include "TypeSystem.hpp"
#include "Variable.hpp"

struct Type;
struct FuncPrototype;
struct ProcPrototype;
struct Variable;

//Scopes own all funcs/structs/traits/etc
struct Scope
{
  Scope(Scope* parent);
  virtual string getLocalName() = 0;
  string getFullPath();               //get full, unambiguous name of scope (for C type names)
  Scope* parent;                      //parent of scope, or NULL for 
  vector<Scope*> children;            //owned scopes
  vector<Type*> types;                //types declared here
  vector<Variable*> vars;             //variables declared here
  //funcs and procs are all declarations and/or definitions in scope
  //definition can go in a parent scope, unless this is a Block
  vector<FuncPrototype*> funcs;
  vector<ProcPrototype*> procs;
};

struct ModuleScope : public Scope
{
  ModuleScope(string name, Scope* parent);
  string getLocalName();
  string name;  //local name
};

struct StructScope : public Scope
{
  StructScope(string name, Scope* parent);
  string getLocalName();
  string name;  //local name
};

struct BlockScope : public Scope
{
  //constructor sets index automatically
  BlockScope(Scope* parent);
  string getLocalName();
  int index;
  static int nextBlockIndex;
};

#endif

