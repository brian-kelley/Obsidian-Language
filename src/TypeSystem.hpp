#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include <string>
#include <iostream>
#include <vector>
#include <map>

#include "Parser.hpp"
#include "Scope.hpp"
#include "TypeSystem.hpp"

/**************************
*   Type System Structs   *
**************************/

struct Scope;
struct TupleType;
struct ArrayType;
struct StructType;
struct UnionType;
struct AliasType;

struct Type
{
  Type(Scope* enclosingScope);
  static void createBuiltinTypes();
  //list of primitive Types corresponding 1-1 with TypeNT::Prim values
  //Get unique, possibly mangled C identifier for use in backend
  Scope* enclosing;
  //T.dimTypes[0] is for T[], T.dimTypes[1] is for T[][], etc.
  vector<Type*> dimTypes;
  //lazily create & return array type for given number of dimensions
  Type* getArrayType(int dims);
  //TODO: whether this can be implicitly converted to other
  virtual bool canConvert(Type* other) = 0;
  //Use this getType() for scope tree building
  static Type* getType(Parser::TypeNT* type, Scope* usedScope);
  //Other variations (so above getType() 
  //"primitives" maps TypeNT::Prim values to corresponding Type*
  static vector<Type*> primitives;
  static vector<TupleType*> tuples;
  static vector<ArrayType*> arrays;
  static vector<Type*> unresolvedTypes;
  static void resolveStruct(StructType* st);
  static void resolveUnion(UnionType* ut);
  static void resolveTuple(TupleType* tt);
  static void resolveAlias(AliasType* at);
  static void resolveArray(ArrayType* at);
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
  StructType(Parser::StructDecl* sd, Scope* enclosingScope, Scope* structScope);
  string name;
  //check for member functions
  //note: self doesn't count as an argument but it is the 1st arg internally
  bool hasFunc(FuncPrototype* type);
  bool hasProc(ProcPrototype* type);
  vector<Trait*> traits;
  vector<Type*> members;
  vector<string> memberNames;
  vector<bool> composed;  //1-1 correspondence with members
  //used to handle unresolved data members
  Parser::StructDecl* decl;
  //member types must be searched from here (the scope inside the struct decl)
  StructScope* structScope;
  bool canConvert(Type* other);
};

struct UnionType : public Type
{
  UnionType(Parser::UnionDecl* ud, Scope* enclosingScope);
  string name;
  vector<Type*> options;
  bool canConvert(Type* other);
};

struct ArrayType : public Type
{
  ArrayType(Type* elemType, int dims);
  Type* elem;
  int dims;
  bool canConvert(Type* other);
};

struct TupleType : public Type
{
  //TupleType has no scope, all are global
  TupleType(vector<Type*> members);
  //Note: TupleType really owned by global scope,
  //but need currentScope to search for member Type*s
  TupleType(Parser::TupleTypeNT* tt, Scope* currentScope);
  vector<Type*> members;
  //this is used only when handling unresolved members
  Parser::TupleTypeNT* decl;
  bool canConvert(Type* other);
};

struct AliasType : public Type
{
  AliasType(Parser::Typedef* td, Scope* enclosingScope);
  AliasType(string alias, Type* underlying, Scope* currentScope);
  string name;
  Type* actual;
  Parser::Typedef* decl;
  bool canConvert(Type* other);
};

struct EnumType : public Type
{
  EnumType(Parser::Enum* e, Scope* enclosingScope);
  string name;
  map<string, int> values;
  bool canConvert(Type* other);
};

struct IntegerType : public Type
{
  IntegerType(string name, int size, bool sign);
  //Size in bytes
  string name;
  int size;
  bool isSigned;
  bool canConvert(Type* other);
};

struct FloatType : public Type
{
  FloatType(string name, int size);
  //4 or 8
  string name;
  int size;
  bool canConvert(Type* other);
};

struct StringType : public Type
{
  StringType();
  bool canConvert(Type* other);
};

struct BoolType : public Type
{
  BoolType();
  bool canConvert(Type* other);
};

#endif

