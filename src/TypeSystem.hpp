#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include <string>
#include <iostream>
#include <vector>
#include <map>

#include "Parser.hpp"
#include "Scope.hpp"

/**************************
*   Type System Structs   *
**************************/

struct Scope;

struct Type
{
  Type(Scope* enclosingScope);
  static void createBuiltinTypes(Scope* global);
  //list of primitive Types corresponding 1-1 with TypeNT::Prim values
  static vector<Type*> primitives;
  //Get unique, possibly mangled C identifier for use in backend
  virtual string getCName() = 0;
  Scope* enclosing;
  //T.dimTypes[0] is for T[], T.dimTypes[1] is for T[][], etc.
  vector<Type*> dimTypes;
  //lazily create & return array type for given number of dimensions
  Type* getArrayType(int dims);
  //Whether this can be implicitly converted to other
  virtual bool canConvert(Type* other);
  static Type* getType(Parser::TypeNT* type, Scope* usedScope);
  static Type* getArrayType(Parser::TypeNT* type, Scope* usedScope, int arrayDims);
  //look up tuple type, or create if doesn't exist
  static Type* getTupleType(Parser::TupleType* tt, Scope* usedScope);
  static Type* getTypeOrUndef(Parser::TypeNT* type, Scope* usedScope, Type* usage);
  //Get non-array, non-tuple type with given name, used in usedScope
  static Type* getType(string localName, Scope* usedScope);
  //Get type by name (with or without scope specifiers)
  //if searchUp, can search in usedScope and then all its parents for the full path of localName
  //otherwise (internal use only): only look in usedScope for localName, not usedScope's parents
  static Type* getType(Parser::Member* localName, Scope* usedScope, bool searchUp = true);
  //all tuple types, can look up by directly comparing components' Type ptrs directly
  static vector<TupleType*> tuples;
};

struct FuncPrototype
{
  FuncPrototype(Parser::FuncType& ft);
  Type* retType;
  vector<Type*> argTypes;
};

struct ProcPrototype
{
  ProcPrototype(Parser::ProcType& pt);
  bool nonterm;
  Type* retType;
  vector<Type*> argTypes;
};

struct Trait
{
  Scope* scope;
  string name;
  vector<FuncPrototype*> funcs;
  vector<ProcPrototype*> procs;
};

struct StructType : public Type
{
  StructType(string name, Scope* enclosingScope);
  StructType(Parser::StructDecl* sd, Scope* enclosingScope);
  string getCName();
  string name;
  //check for member functions
  //note: self doesn't count as an argument but it is the 1st arg internally
  bool hasFunc(ProcType& type);
  bool hasProc(ProcType& type);
  vector<AP(Trait)> traits;
  vector<AP(Type)> members;
  vector<bool> composed;  //1-1 correspondence with members
};

struct UnionType : public Type
{
  UnionType(Parser::UnionDecl* ud, Scope* enclosingScope);
  string getCName();
  string name;
  vector<AP(Type*)> options;
};

struct TupleType : public Type
{
  //TupleType has no scope, all are global
  TupleType(vector<Type*> members);
  TupleType(Parser::TupleType* tt);
  string getCName();
  vector<Type*> members;
};

struct AliasType : public Type
{
  AliasType(string newName, Type* t, Scope* enclosingScope);
  AliasType(Parser::Typedef* td, Scope* enclosingScope);
  string getCName();
  Type* actual;
};

struct EnumType : public Type
{
  EnumType(Parser::Enum* e, Scope* enclosingScope);
  string getCName();
  string name;
  map<string, int> values;
};

struct IntegerType : public Type
{
  IntegerType(string name, int size, bool sign);
  string getCName();
  //Size in bytes
  string name;
  int size;
  bool isSigned;
};

struct FloatType : public Type
{
  FloatType(string name, int size);
  //4 or 8
  string name;
  int size;
  string getCName();
};

struct StringType : public Type
{
  string getCName();
};

struct BoolType : public Type
{
  string getCName();
};

//Undef type: need a placeholder for types not yet defined
struct UndefType : public Type
{
  UndefType(Parser::TypeNT* t, Scope* enclosing, Type* usage);
  UndefType(string name, Scope* enclosing, Type* usage);
  string getCName();
  string localName;
  variant<StructType*, UnionType*, TupleType*, ArrayType*> 
  enum UsageType
  {
    STRUCT,
    UNION,
    TUPLE,
    ARRAY
  };
  //keep track of all known undefined types so they can be resolved more quickly
  static vector<UndefType*> all;
};

#endif

