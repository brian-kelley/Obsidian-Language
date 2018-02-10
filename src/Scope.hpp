#ifndef SCOPE_H
#define SCOPE_H

#include "Common.hpp"
#include "AST.hpp"

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
    ENUM_CONSTANT,
    META_VAR
  };
  Name() : item(nullptr), kind(NONE), scope(nullptr) {}
  Name(Module* m, Scope* parent)
    : item(m), kind(MODULE), scope(parent) {}
  Name(Struct* st, Scope* s)
    : item(st), kind(STRUCT), scope(s) {}
  Name(Enum* e, Scope* s)
    : item(e), kind(ENUM), scope(s) {}
  Name(Alias* a, Scope* s)
    : item(a), kind(TYPEDEF), scope(s) {}
  Name(Subroutine* subr, Scope* s)
    : item(subr), kind(SUBROUTINE), scope(s) {}
  Name(ExternalSubroutine* subr, Scope* s)
    : item(subr), kind(EXTERN_SUBR), scope(s) {}
  Name(Variable* var, Scope* s)
    : item(var), kind(VARIABLE), scope(s) {}
  Name(EnumConstant* ec, Scope* s)
    : item(ec), kind(ENUM_CONSTANT), scope(s) {}
  Name(MetaVar* var, Scope* s)
    : item(var), kind(META_VAR), scope(s) {}
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
  void addName(Name n);
  map<string, Name> names;
  private:
  //make sure that name won't shadow any existing declaration
  void shadowCheck(string name);
};

#endif

