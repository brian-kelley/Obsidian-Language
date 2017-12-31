#ifndef SCOPE_H
#define SCOPE_H

#include "Parser.hpp"
#include "Common.hpp"

//Forward-declare all the things that Scopes contain
namespace TypeSystem
{
  struct Type;
  struct StructType;
  struct EnumType;
  struct EnumConstant;
  struct AliasType;
  struct BoundedType;
  struct Trait;
  struct TType;
}

struct Subroutine;
struct Variable;
struct Scope;
struct ModuleScope;
struct StructScope;

// Unified name lookup system
struct Name
{
  enum Kind
  {
    NONE,
    MODULE,
    STRUCT,
    ENUM,
    TYPEDEF,
    BOUNDED_TYPE,
    TRAIT,
    SUBROUTINE,
    VARIABLE,
    ENUM_CONSTANT
  };
  Name() : item(nullptr), kind(NONE), scope(nullptr) {}
  Name(ModuleScope* m, Scope* parent)
    : item(m), kind(MODULE), scope(parent) {}
  Name(TypeSystem::StructType* st, Scope* s)
    : item(st), kind(STRUCT), scope(s) {}
  Name(TypeSystem::EnumType* e, Scope* s)
    : item(e), kind(ENUM), scope(s) {}
  Name(TypeSystem::AliasType* a, Scope* s)
    : item(a), kind(TYPEDEF), scope(s) {}
  Name(TypeSystem::BoundedType* b, Scope* s)
    : item(b), kind(BOUNDED_TYPE), scope(s) {}
  Name(TypeSystem::Trait* t, Scope* s)
    : item(t), kind(TRAIT), scope(s) {}
  Name(Subroutine* subr, Scope* s)
    : item(subr), kind(SUBROUTINE), scope(s) {}
  Name(Variable* var, Scope* s)
    : item(var), kind(VARIABLE), scope(s) {}
  Name(TypeSystem::EnumConstant* ec, Scope* s)
    : item(ec), kind(ENUM_CONSTANT), scope(s) {}
  void* item;
  //All named declaration types
  Kind kind;
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
  //unified name handling
  Name findName(Parser::Member* mem);
  Name findName(string name);
  Name lookup(string name);
  void addName(ModuleScope* m);
  void addName(TypeSystem::StructType* st);
  void addName(TypeSystem::EnumConstant* ec);
  void addName(TypeSystem::EnumType* et);
  void addName(TypeSystem::AliasType* at);
  void addName(TypeSystem::BoundedType* bt);
  void addName(TypeSystem::Trait* t);
  void addName(Subroutine* s);
  void addName(Variable* v);
  map<string, Name> names;
  private:
  //make sure that name won't shadow any existing declaration
  void shadowCheck(string name);
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
  TypeSystem::StructType* type;
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

struct SubroutineScope : public Scope
{
  //constructor sets index automatically
  //also makes astIn point back to this
  SubroutineScope(Scope* parent) : Scope(parent), subr(nullptr) {}
  string getLocalName();  //local name uses index to produce a unique name
  Subroutine* subr;
};

//Need a scope for traits so that T can be created locally as a type
struct TraitScope : public Scope
{
  TraitScope(Scope* parent, string n);
  TypeSystem::Trait* trait;
  TypeSystem::TType* ttype;
  string getLocalName();
  string name; //local name
};

#endif

