#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include <string>
#include <iostream>
#include <vector>
#include <map>

#include "Parser.hpp"

/**************************
*   Type System Structs   *
**************************/

struct Scope;
struct TupleType;
struct ArrayType;

struct Type
{
  Type(Scope* enclosingScope);
  static void createBuiltinTypes(Scope* global);
  //list of primitive Types corresponding 1-1 with TypeNT::Prim values
  //Get unique, possibly mangled C identifier for use in backend
  Scope* enclosing;
  //T.dimTypes[0] is for T[], T.dimTypes[1] is for T[][], etc.
  vector<Type*> dimTypes;
  //lazily create & return array type for given number of dimensions
  Type* getArrayType(int dims);
  //TODO: whether this can be implicitly converted to other
  virtual bool canConvert(Type* other);
  //Use this getType() for scope tree building
  static Type* getType(Parser::TypeNT* type, Scope* usedScope);
  //Other variations (so above getType() 
  //"primitives" maps TypeNT::Prim values to corresponding Type*
  static vector<Type*> primitives;
  static vector<TupleType*> tuples;
  static vector<ArrayType*> arrays;
  static vector<Type*> unresolvedTypes;
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
  string name;
  //check for member functions
  //note: self doesn't count as an argument but it is the 1st arg internally
  bool hasFunc(FuncPrototype& type);
  bool hasProc(ProcPrototype& type);
  vector<AP(Trait)> traits;
  vector<AP(Type)> members;
  vector<bool> composed;  //1-1 correspondence with members
  //used to handle unresolved data members
  Parser::StructDecl* decl;
};

struct UnionType : public Type
{
  UnionType(Parser::UnionDecl* ud, Scope* enclosingScope);
  string name;
  vector<AP(Type*)> options;
};

struct TupleType : public Type
{
  //TupleType has no scope, all are global
  TupleType(vector<Type*> members);
  TupleType(Parser::TupleTypeNT* tt);
  vector<Type*> members;
  //this is used only when handling unresolved members
  Parser::TupleTypeNT* decl;
};

struct AliasType : public Type
{
  AliasType(Parser::Typedef* td, Scope* enclosingScope);
  string name;
  Type* actual;
  Parser::Typedef* decl;
};

struct EnumType : public Type
{
  EnumType(Parser::Enum* e, Scope* enclosingScope);
  string name;
  map<string, int> values;
};

struct IntegerType : public Type
{
  IntegerType(string name, int size, bool sign);
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
};

struct StringType : public Type
{
};

struct BoolType : public Type
{
};

#endif

