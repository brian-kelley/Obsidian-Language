#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <cassert>

#include "Parser.hpp"
#include "Scope.hpp"
#include "TypeSystem.hpp"
#include "AST_Printer.hpp"

/* Type system: 3 main categories of types
 *  -Primitives
 *    -belong to global scope
 *    -names are keywords
 *    -created before all other scope/type loading
 *  -Named types: struct, union, enum, typedef, func, proc
 *    -created immediately when encountered in scope tree loading
 *    -If dependent on any unavailable type, remember to resolve later
 *  -Unnamed (syntactic) types: array and tuple
 *    -Created last, as necessary, to resolve all named types
 *    -Belong to global scope
 *    -Never duplicated, to save memory and speed up comparison (e.g. at most one int[] or (int, int) type defined in whole program)
 *    -For aliases: always trace back to underlying type (i32[][] becomes int[][])
 */

/**************************
*   Type System Structs   *
**************************/

struct Scope;
struct StructScope;
struct TupleType;
struct ArrayType;
struct StructType;
struct UnionType;
struct AliasType;
struct Type;

//  UnresolvedType is used to remember an instance of a type (used in another type)
//  that cannot be resolved during the first pass
//
//  -Parsed is the original AST node that represents the type that couldn't be looked up
//  -Usage is where the type ID is needed by some other dependent type
struct UnresolvedType
{
  //UnresolvedType() : parsed(NULL), usage(NULL) {}
  UnresolvedType(Parser::TypeNT* t, Scope* s, Type** u) : parsed(t), scope(s), usage(u) {}
  Parser::TypeNT* parsed;
  Scope* scope;
  Type** usage;
};

struct Type
{
  Type(Scope* enclosingScope);
  static void createBuiltinTypes();
  //list of primitive Types corresponding 1-1 with TypeNT::Prim values
  Scope* enclosing;
  //resolve all types that couldn't be found during first pass
  static void resolveAll();
  //dimTypes[0] is for T[], dimTypes[1] is for T[][], etc.
  vector<Type*> dimTypes;
  // [lazily create and] return array type for given number of dimensions of *this
  Type* getArrayType(int dims);
  //TODO: whether this can be implicitly converted to other
  virtual bool canConvert(Type* other) = 0;
  //Use this getType() for scope tree building
  //Need "usage" so 2nd pass of type resolution can directly assign the resolved type
  static Type* getType(Parser::TypeNT* type, Scope* usedScope, Type** usage, bool failureIsError = true);
  //Other variations (so above getType() 
  //"primitives" maps TypeNT::Prim values to corresponding Type*
  static vector<Type*> primitives;
  //quickly lookup (or create) unique TupleType for given members
  TupleType* lookupTuple(vector<Type*>& members);
  //Note: the set stores TupleType pointers, but compares them by operator< on the dereferenced values
  static vector<TupleType*> tuples;
  static vector<ArrayType*> arrays;
  static vector<UnresolvedType> unresolved;
  virtual bool isArray();
  virtual bool isStruct();
  virtual bool isUnion();
  virtual bool isTuple();
  virtual bool isEnum();
  virtual bool isCallable();
  virtual bool isProc();
  virtual bool isFunc();
  virtual bool isInteger();
  virtual bool isNumber();
  virtual bool isString();
  virtual bool isBool();
};

struct FuncPrototype : public Type
{
  FuncPrototype(Parser::FuncType& ft);
  Type* retType;
  vector<Type*> argTypes;
};

struct ProcPrototype : public Type
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
  StructType(Parser::StructDecl* sd, Scope* enclosingScope, StructScope* structScope);
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
  bool isStruct();
};

struct UnionType : public Type
{
  UnionType(Parser::UnionDecl* ud, Scope* enclosingScope);
  string name;
  vector<Type*> options;
  Parser::UnionDecl* decl;
  bool canConvert(Type* other);
  bool isUnion();
};

struct ArrayType : public Type
{
  //note: dims in type passed to ctor ignored
  ArrayType(Type* elemType, int dims);
  Type* elem;
  Parser::TypeNT* elemNT;
  int dims;
  bool canConvert(Type* other);
  bool isArray();
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
  bool isTuple();
  //Whether this->members exactly matches types
  bool matchesTypes(vector<Type*>& types);
  bool operator<(const TupleType& rhs)
  {
    return lexicographical_compare(members.begin(), members.end(), rhs.members.begin(), rhs.members.end());
  }
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
  bool isEnum();
  bool isInteger();
  bool isNumber();
};

struct IntegerType : public Type
{
  IntegerType(string name, int size, bool sign);
  //Size in bytes
  string name;
  int size;
  bool isSigned;
  bool canConvert(Type* other);
  bool isInteger();
  bool isNumber();
};

struct FloatType : public Type
{
  FloatType(string name, int size);
  //4 or 8
  string name;
  int size;
  bool canConvert(Type* other);
  bool isNumber();
};

struct StringType : public Type
{
  StringType();
  bool canConvert(Type* other);
  bool isString();
};

struct BoolType : public Type
{
  BoolType();
  bool canConvert(Type* other);
  bool isBool();
};

#endif

