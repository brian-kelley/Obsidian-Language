#ifndef SCOPE_H
#define SCOPE_H

#include "Common.hpp"
#include "AST.hpp"

namespace TypeSystem
{
  struct StructType;
}

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
    SUBROUTINE,
    EXTERN_SUBR,
    VARIABLE,
    ENUM_CONSTANT
  };
  Name() : item(nullptr), kind(NONE), scope(nullptr) {}
  Name(Module* m, Scope* parent)
    : item(m), kind(MODULE), scope(parent), node(m) {}
  Name(StructType* st, Scope* s)
    : item(st), kind(STRUCT), scope(s), node(st) {}
  Name(Enum* e, Scope* s)
    : item(e), kind(ENUM), scope(s), node(e) {}
  Name(Alias* a, Scope* s)
    : item(a), kind(TYPEDEF), scope(s), node(a) {}
  Name(Subroutine* subr, Scope* s)
    : item(subr), kind(SUBROUTINE), scope(s), node(subr){}
  Name(ExternalSubroutine* subr, Scope* s)
    : item(subr), kind(EXTERN_SUBR), scope(s), node(subr) {}
  Name(Variable* var, Scope* s)
    : item(var), kind(VARIABLE), scope(s), node(var) {}
  Name(EnumConstant* ec, Scope* s)
    : item(ec), kind(ENUM_CONSTANT), scope(s), node(ec) {}
  void* item;
  //All named declaration types
  Kind kind;
  Scope* scope;
  Node* node;
  bool inScope(Scope* s);
};

struct Module
{
  //name is "" for global scope
  string name;
  Scope* scope;
};

//Scopes own all funcs/structs/traits/etc
struct Scope
{
  Scope(Scope* parent, Module* m);
  Scope(Scope* parent, StructType* s);
  Scope(Scope* parent, Subroutine* s);
  Scope(Scope* parent, Block* b);
  Scope(Scope* parent, Enum* e);
  virtual string getLocalName() = 0;
  string getFullPath();               //get full, unambiguous name of scope (for C type names)
  Scope* parent;                      //parent of scope, or NULL for 
  Name findName(Parser::Member* mem);
  //try to find name in this scope or a parent scope
  Name findName(string name);
  //try to find name in this scope only
  Name lookup(string name);
  void addName(Name n);
  map<string, Name> names;
  //if in static context, this returns NULL
  //otherwise, returns the Struct that "this" would refer to
  TypeSystem::StructType* getStructContext();
  //if in a struct (or module within struct) return the struct
  //otherwise NULL
  TypeSystem::StructType* getMemberContext();
  /*  take innermost function scope
      if static, return that function's scope
      if member, return owning struct
      otherwise return NULL

      This is used for purity checking
  */
  Scope* getFunctionContext();
  //does this contain other?
  bool contains(Scope* other);
  //all types that can represent a Scope in the AST
  //using this variant instead of having these types inherit Scope
  variant<Module*, StructType*, Subroutine*, Block*, Enum*> node;
};

extern Scope* global;

#endif

