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

//need to forward-declare this to resolve mutual dependency
struct Expression;

namespace TypeSystem
{

struct Type;
struct TupleType;
struct ArrayType;
struct StructType;
struct UnionType;
struct AliasType;
struct FuncType;
struct ProcType;
struct Trait;

//  UnresolvedType is used to remember an instance of a type (used in another type)
//  that cannot be resolved during the first pass
//
//  -parsed[...] is the original AST node that represents the type that couldn't be looked up
//  -Usage is where the type ID is needed by some other dependent type
struct UnresolvedType
{
  //UnresolvedType() : parsed(NULL), usage(NULL) {}
  UnresolvedType(Parser::TypeNT* t, Scope* s, Type** u) : scope(s), usage(u)
  {
    parsedType = t;
    parsedFunc = nullptr;
    parsedProc = nullptr;
  }
  UnresolvedType(Parser::FuncTypeNT* t, Scope* s, Type** u) : scope(s), usage(u)
  {
    parsedType = nullptr;
    parsedFunc = t;
    parsedProc = nullptr;
  }
  UnresolvedType(Parser::ProcTypeNT* t, Scope* s, Type** u) : scope(s), usage(u)
  {
    parsedType = nullptr;
    parsedFunc = nullptr;
    parsedProc = t;
  }
  Parser::TypeNT* parsedType;
  Parser::FuncTypeNT* parsedFunc;
  Parser::ProcTypeNT* parsedProc;
  Scope* scope;
  Type** usage;
};

struct UnresolvedTrait
{
  //UnresolvedType() : parsed(NULL), usage(NULL) {}
  UnresolvedTrait(Parser::Member* t, Scope* s, Trait** u) : parsed(t), scope(s), usage(u) {}
  Parser::Member* parsed;
  Scope* scope;
  Trait** usage;
};

//If inTrait, "T" refers to TType
Type* getType(Parser::TypeNT* type, Scope* usedScope, Type** usage, bool failureIsError = true);
FuncType* getFuncType(Parser::FuncTypeNT* type, Scope* usedScope, Type** usage, bool failureIsError = true);
ProcType* getProcType(Parser::ProcTypeNT* type, Scope* usedScope, Type** usage, bool failureIsError = true);
Trait* getTrait(Parser::Member* name, Scope* usedScope, Trait** usage, bool failureIsError = true);
Type* getIntegerType(int bytes, bool isSigned);
void resolveAllTypes();
void resolveAllTraits();
void createBuiltinTypes();

extern vector<TupleType*> tuples;
extern vector<ArrayType*> arrays;
extern vector<UnresolvedType> unresolved;
extern vector<UnresolvedTrait> unresolvedTraits;
extern vector<Type*> primitives;
extern Type* self;

struct Type
{
  Type(Scope* enclosingScope);
  //list of primitive Types corresponding 1-1 with TypeNT::Prim values
  Scope* enclosing;
  //resolve all types that couldn't be found during first pass
  //dimTypes[0] is for T[], dimTypes[1] is for T[][], etc.
  vector<Type*> dimTypes;
  // [lazily create and] return array type for given number of dimensions of *this
  Type* getArrayType(int dims);
  //get integer type corresponding to given size (bytes) and signedness
  virtual bool canConvert(Type* other) = 0;
  virtual bool canConvert(Expression* other)
  {
    //Basic behavior here: if other has a known type, check if that can convert
    if(other->type)
      return canConvert(other->type);
    return false;
  }
  //Use this getType() for scope tree building
  //Need "usage" so 2nd pass of type resolution can directly assign the resolved type
  //Other variations (so above getType() 
  //"primitives" maps TypeNT::Prim values to corresponding Type*
  //quickly lookup (or create) unique TupleType for given members
  TupleType* lookupTuple(vector<Type*>& members);
  //Note: the set stores TupleType pointers, but compares them by operator< on the dereferenced values
  virtual bool isArray()    {return false;}
  virtual bool isStruct()   {return false;}
  virtual bool isUnion()    {return false;}
  virtual bool isTuple()    {return false;}
  virtual bool isEnum()     {return false;}
  virtual bool isCallable() {return false;}
  virtual bool isProc()     {return false;}
  virtual bool isFunc()     {return false;}
  virtual bool isInteger()  {return false;}
  virtual bool isNumber()   {return false;}
  virtual bool isString()   {return false;}
  virtual bool isBool()     {return false;}
  virtual bool isConcrete() {return true;}
};

struct FuncType : public Type
{
  FuncType(Parser::FuncTypeNT* ft, Scope* usedScope);
  Type* retType;
  vector<Type*> argTypes;
  bool isCallable();
  bool isFunc();
  bool canConvert(Type* other);
};

struct ProcType : public Type
{
  ProcType(Parser::ProcTypeNT* pt, Scope* usedScope);
  bool nonterm;
  Type* retType;
  vector<Type*> argTypes;
  bool isCallable();
  bool isProc();
  bool canConvert(Type* other);
};

//Function/Prod
struct FunctionDecl
{
  FuncType* type;
  string name;
};

struct ProcedureDecl
{
  ProcType* type;
  string name;
};

struct Trait
{
  Trait(Parser::TraitDecl* td, Scope* s);
  Scope* scope;
  string name;
  //Trait is a set of named function and procedure types
  vector<FunctionDecl> funcs;
  vector<ProcedureDecl> procs;
};

//Bounded type: a set of traits that define a polymorphic argument type (like Java)
//  ex: func string doThing(T: Printable val);
//Can only be used in argument lists.
//When polymorphic callable is instantiated,
//the BoundedType becomes an alias type within the function
struct BoundedType : public Type
{
  //a bounded type is called TraitType in the parser
  BoundedType(Parser::TraitType* tt, Scope* s);
  vector<Trait*> traits;
  string name;
  bool canConvert(Type* other)
  {
    return false;
  }
  bool isConcrete()
  {
    return false;
  }
};

struct StructType : public Type
{
  StructType(Parser::StructDecl* sd, Scope* enclosingScope, StructScope* structScope);
  string name;
  //check for member functions
  //note: self doesn't count as an argument but it is the 1st arg internally
  bool hasFunc(FuncType* type);
  bool hasProc(ProcType* type);
  vector<Trait*> traits;
  vector<Type*> members;
  vector<string> memberNames;
  vector<bool> composed;  //1-1 correspondence with members
  //used to handle unresolved data members
  Parser::StructDecl* decl;
  //member types must be searched from here (the scope inside the struct decl)
  StructScope* structScope;
  bool canConvert(Type* other);
  bool canConvert(Expression* other);
  bool isStruct();
};

struct UnionType : public Type
{
  UnionType(Parser::UnionDecl* ud, Scope* enclosingScope);
  string name;
  vector<Type*> options;
  Parser::UnionDecl* decl;
  bool canConvert(Type* other);
  bool canConvert(Expression* other);
  bool isUnion();
};

struct ArrayType : public Type
{
  ArrayType(Type* elemType, int dims);
  Type* elem;
  Parser::TypeNT* elemNT;
  int dims;
  bool canConvert(Type* other);
  bool canConvert(Expression* other);
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
  bool canConvert(Expression* other);
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
  bool canConvert(Expression* other);
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
  string name;
  //4 or 8 (bytes, not bits)
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

struct TType : public Type
{
  TType();
  static TType inst;
  bool canConvert(Type* other);
};

} //namespace TypeSystem

#endif

