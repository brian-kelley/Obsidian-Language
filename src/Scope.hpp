#ifndef SCOPE_H
#define SCOPE_H

#include "Parser.hpp"
#include "Common.hpp"

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
struct Scope;
struct StructType;

// Unified name lookup system
struct Name
{
  enum TYPE
  {
    SCOPE,
    STRUCT,
    UNION,
    ENUM,
    TYPEDEF,
    TRAIT,
    SUBROUTINE,
    VARIABLE
  };
  Name() : item(nullptr), type(SCOPE) {}
  Name(void* ptr, TYPE t, Scope* s) : item(ptr), type(t), scope(s) {}
  void* item;
  //All named declaration types
  TYPE type;
  Scope* scope;
};

//Scopes own all funcs/structs/traits/etc
struct Scope
{
  Scope(Scope* parent);
  virtual string getLocalName() = 0;
  string getFullPath();               //get full, unambiguous name of scope (for C type names)
  Scope* parent;                      //parent of scope, or NULL for 
  vector<Scope*> children;            //owned scopes
  vector<TypeSystem::Type*> types;    //named types (struct, enum, alias, bounded type)
  vector<TypeSystem::Trait*> traits;
  vector<Variable*> vars;             //variables declared here - first globals & statics and then locals (in order of declaration)
  //subroutines (funcs and procs) defined in scope
  vector<Subroutine*> subr;
  //Find a sub scope of this (or a parent) with given relative "path"
  //"names" will probably come from Parser::Member::scopes
  vector<Scope*> findSub(vector<string>& names);
  //Look up types, variables, subroutines (return NULL if not found, or wrong type)
  TypeSystem::Type* findType(Parser::Member* mem);
  Variable* findVariable(Parser::Member* mem);
  TypeSystem::Trait* findTrait(Parser::Member* mem);
  Subroutine* findSubroutine(Parser::Member* mem);
  //unified name handling
  map<string, Name> names;
  //add name to scope
  template<typename Decl> void addName(Decl* d);
  //look up a name until a non-scope item is reached
  //return the name and provide the remaining compound ident
  bool lookup(vector<string> names, Name& found, vector<string>& remain);
  private:
  void findSubImpl(vector<string>& names, vector<Scope*>& matches);
};

struct ModuleScope : public Scope
{
  ModuleScope(string name, Scope* parent, Parser::Module* astIn);
  string getLocalName();
  string name;  //local name
};

struct StructScope : public Scope
{
  StructScope(string name, Scope* parent, Parser::StructDecl* astIn);
  StructType* type;
  string getLocalName();
  string name;  //local name
};

struct BlockScope : public Scope
{
  //constructor sets index automatically
  //also makes astIn point back to this
  BlockScope(Scope* parent, Parser::Block* astIn);
  BlockScope(Scope* parent);
  string getLocalName();  //local name uses index to produce a unique name
  int index;
  static int nextBlockIndex;
};

//Need a scope for traits so that T can be created locally as a type
struct TraitScope : public Scope
{
  TraitScope(Scope* parent, string n);
  string getLocalName();
  string name; //local name
};

#endif

