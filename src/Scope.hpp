#ifndef SCOPE_H
#define SCOPE_H

#include "Common.hpp"
#include "AST.hpp"

struct Scope;
struct StructType;
struct AliasType;
struct EnumType;
struct EnumConstant;
struct Subroutine;
struct ExternalSubroutine;
struct Variable;
struct Block;
struct Member;

struct Module : public Node
{
  //name is "" for global scope
  Module(string n, Scope* s);
  string name;
  //scope->node == this
  Scope* scope;
};

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
  Name(Module* m, Scope* parent);
  Name(StructType* st, Scope* s);
  Name(EnumType* e, Scope* s);
  Name(AliasType* a, Scope* s);
  Name(Subroutine* subr, Scope* s);
  Name(ExternalSubroutine* subr, Scope* s);
  Name(Variable* var, Scope* s);
  Name(EnumConstant* ec, Scope* s);
  Node* item;
  //All named declaration types
  Kind kind;
  Scope* scope;
  bool inScope(Scope* s);
};

//Scopes own all funcs/structs/traits/etc
struct Scope
{
  Scope(Scope* parent, Module* m);
  Scope(Scope* parent, StructType* s);
  Scope(Scope* parent, Subroutine* s);
  Scope(Scope* parent, Block* b);
  Scope(Scope* parent, EnumType* e);
  virtual string getLocalName() = 0;
  string getFullPath();               //get full, unambiguous name of scope (for C type names)
  Scope* parent;                      //parent of scope, or NULL for 
  Name findName(Member* mem);
  //try to find name in this scope or a parent scope
  Name findName(string name);
  //try to find name in this scope only
  Name lookup(string name);
  void addName(Name n);
  void addName(Variable* v);
  void addName(Module* m);
  void addName(StructType* s);
  void addName(Subroutine* s);
  void addName(AliasType* a);
  void addName(ExternalSubroutine* s);
  void addName(EnumType* e);
  void addName(EnumConstant* e);
  map<string, Name> names;
  //if in static context, this returns NULL
  //otherwise, returns the Struct that "this" would refer to
  StructType* getStructContext();
  //if in a struct (or module within struct) return the struct
  //otherwise NULL
  StructType* getMemberContext();
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
  variant<Module*, StructType*, Subroutine*, Block*, EnumType*> node;
};

extern Scope* global;

#endif

