#ifndef SCOPE_H
#define SCOPE_H

#include "Common.hpp"
#include "AST.hpp"

struct Scope;
struct Module;
struct UsingDecl;
struct StructType;
struct AliasType;
struct EnumType;
struct EnumConstant;
struct SimpleType;
struct SubroutineDecl;
struct Subroutine;
struct Variable;
struct Block;
struct SourceFile;

extern Module* global;

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
    SIMPLE_TYPE,
    SUBROUTINE,
    VARIABLE,
    ENUM_CONSTANT
  };
  Name() : item(nullptr), kind(NONE), name(""), scope(nullptr) {}
  Name(Module* m, Scope* parent);
  Name(StructType* st, Scope* s);
  Name(EnumType* e, Scope* s);
  Name(SimpleType* t, Scope* s);
  Name(AliasType* a, Scope* s);
  Name(SubroutineDecl* sd, Scope* s);
  Name(Variable* var, Scope* s);
  Name(EnumConstant* ec, Scope* s);
  Node* item;
  //All named declaration types
  Kind kind;
  string name;
  Scope* scope;
  bool inScope(Scope* s);
};

struct Module : public Node
{
  //name is "" for global scope
  Module(string n, Scope* s);
  void resolveImpl();
  //table of files that have been included in this module
  string name;
  //scope->node == this
  Scope* scope;
};

//Scopes own all funcs/structs/traits/etc
struct Scope
{
  Scope(Scope* parent, Module* m);
  Scope(Scope* parent, StructType* s);
  Scope(Scope* parent, Subroutine* s);
  Scope(Scope* parent, Block* b);
  Scope(Scope* parent, EnumType* e);
  string getLocalName();
  string getFullPath();               //get full, unambiguous name of scope (for C type names)
  Scope* parent;                      //parent of scope, or NULL for 
  Name findName(Member* mem, bool allowUsing = true);
  //try to find name in this scope or any parent scope
  Name findName(const string& name, bool allowUsing = true);
  //try to find name in this scope only
  Name lookup(const string& name, bool allowUsing = true);
  void addName(const Name& n);
  void addName(Variable* v);
  void addName(Module* m);
  void addName(StructType* s);
  void addName(SubroutineDecl* sf);
  void addName(AliasType* a);
  void addName(SimpleType* s);
  void addName(EnumType* e);
  void addName(EnumConstant* e);
  //Resolving all UsingDecls in this and all child scopes
  void resolveAllUsings();
  //Resolve all names (in this scope only)
  void resolveAll();
  map<string, Name> names;
  vector<UsingDecl*> usingDecls;
  vector<Scope*> children;
  //Returns the StructType that "this" would refer to.
  StructType* getStructContext();
  //For a non-static variable declared in this scope, determine the StructType
  //it would become a member of (if any)
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
  //is this a module or submodule in global scope?
  bool isNestedModule();
  //Visit each scope (DFS) in the program
  template<typename F>
  static void walk(F f)
  {
    vector<Scope*> visit;
    visit.push_back(global->scope);
    while(visit.size())
    {
      Scope* s = visit.back();
      f(s);
      visit.pop_back();
      for(auto child : s->children)
      {
        visit.push_back(child);
      }
    }
  }
  //all types that can represent a Scope in the AST
  //using this variant instead of having these types inherit Scope
  variant<Module*, StructType*, Subroutine*, Block*, EnumType*> node;
};

struct UsingDecl : public Node
{
  virtual Name lookup(const string& n) = 0;
  virtual void resolveImpl() = 0;
};

struct UsingModule : public UsingDecl
{
  UsingModule(Member* mname, Scope* s);
  void resolveImpl();
  Name lookup(const string& n);
private:
  //Before resolving:
  Member* moduleName;
  Scope* scope;
  //After resolving:
  Module* module;
};

struct UsingName : public UsingDecl
{
  UsingName(Member* n, Scope* s);
  void resolveImpl();
  Name lookup(const string& n);
private:
  //Before resolving:
  Member* fullName;
  Scope* scope;
  //After resolving:
  Name name;
};

#endif

