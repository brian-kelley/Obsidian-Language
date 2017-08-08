#ifndef SCOPE_H
#define SCOPE_H

#include <iostream>
#include <string>
#include <vector>

//Forward-declare all the things that Scopes contain
namespace TypeSystem
{
  struct Type;
  struct FuncType;
  struct ProcType;
  struct Trait;
}

struct Subroutine;
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
  vector<Variable*> vars;             //variables declared here - first globals & statics and then locals (in order of declaration)
  //subroutines (funcs and procs) defined in scope
  vector<Subroutine*> subr;
  //Find a sub scope of this (or a parent) with given relative "path"
  //"names" will probably come from Parser::Member::scopes
  vector<Scope*> findSub(vector<string>& names);
  //Look up types, variables, subroutines (return NULL if not found)
  TypeSystem::Type* findType(Parser::Member* mem);
  Variable* findVariable(Parser::Member* mem);
  Trait* findTrait(Parser::Member* mem);
  Subroutine* findSubroutine(Parser::Member* mem);
  private:
  void findSubImpl(vector<string>& names, vector<Scope*>& matches);
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
  BlockScope(Scope* parent);
  string getLocalName();
  Parser::Block* ast;
  int index;
  string getLocalName();
  Parser::Block* ast;
  int index;
  static int nextBlockIndex;
};

#endif

