#ifndef SCOPE_H
#define SCOPE_H

#include <iostream>
#include <string>
#include <vector>

#include "AutoPtr.hpp"
#include "TypeSystem.hpp"
#include "Variable.hpp"

namespace TypeSystem
{
  struct Type;
  struct FuncType;
  struct ProcType;
  struct Trait;
}

struct Variable;

//Scopes own all funcs/structs/traits/etc
struct Scope
{
  Scope(Scope* parent);
  virtual string getLocalName() = 0;
  string getFullPath();               //get full, unambiguous name of scope (for C type names)
  Scope* parent;                      //parent of scope, or NULL for 
  vector<Scope*> children;            //owned scopes
  vector<TypeSystem::Type*> types;    //named types declared here (struct, enum, union, etc)
  vector<TypeSystem::Trait*> traits;  //traits declared here
  vector<Variable*> vars;             //variables declared here
  //funcs and procs are all fully implemented functions in a scope
  //Struct member funcs/procs can be declared before defined but then they must be defined in parent scope
  vector<TypeSystem::FuncType*> funcs;
  vector<TypeSystem::ProcType*> procs;
};

struct ModuleScope : public Scope
{
  ModuleScope(string name, Scope* parent, Parser::Module* astIn);
  string getLocalName();
  Parser::Module* ast;
  string name;  //local name
};

struct StructScope : public Scope
{
  StructScope(string name, Scope* parent, Parser::StructDecl* astIn);
  string getLocalName();
  Parser::StructDecl* ast;
  string name;  //local name
};

struct BlockScope : public Scope
{
  //constructor sets index automatically
  BlockScope(Scope* parent, Parser::Block* astIn);
  string getLocalName();
  Parser::Block* ast;
  int index;
  static int nextBlockIndex;
};

#endif

