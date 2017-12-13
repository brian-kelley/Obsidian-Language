#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include "Parser.hpp"
#include "Scope.hpp"
#include "TypeSystem.hpp"
#include "DeferredLookup.hpp"

/* Type system: 3 main categories of types
 *  -Primitives
 *    -don't belong to any scope (type lookup checks primitives first)
 *    -created before all other scope/type loading
 *  -Named types: struct, union, enum, typedef/alias
 *    -created immediately when encountered in scope tree loading
 *    -belong to the type where declared
 *  -Unnamed (syntactic) types: array and tuple
 *    -don't belong to any scope
 */

/**************************
*   type system structs   *
**************************/

struct Scope;
struct StructScope;

//need to forward-declare this to resolve mutual dependency
struct Expression;

namespace TypeSystem
{

struct Type;
struct Trait;
struct StructType;
struct TupleType;
struct ArrayType;
struct UnionType;
struct MapType;
struct AliasType;
struct BoundedType;

struct CallableType;

struct TypeLookup
{
  TypeLookup(Parser::TypeNT* t, Scope* s) : type(t), scope(s) {}
  TypeLookup(Parser::SubroutineTypeNT* t, Scope* s) : type(t), scope(s) {}
  //Even though SubroutineTypeNT is an option for TypeNT,
  //need it here separately for looking up trait subroutine types
  variant<Parser::TypeNT*,
          Parser::SubroutineTypeNT*> type;
  Scope* scope;
};

struct TraitLookup
{
  TraitLookup(Parser::Member* n, Scope* s) : name(n), scope(s) {}
  Parser::Member* name;
  Scope* scope;
};

//type error message function, to be used by DeferredLookup on types
string typeErrorMessage(TypeLookup& lookup);
string traitErrorMessage(TraitLookup& lookup);

Type* lookupType(Parser::TypeNT* type, Scope* scope);
CallableType* lookupSubroutineType(Parser::SubroutineTypeNT* subr, Scope* scope);
//wrapper for lookupType used by deferred type lookup
Type* lookupTypeDeferred(TypeLookup& args);

Trait* lookupTrait(Parser::Member* type, Scope* scope);
Trait* lookupTraitDeferred(TraitLookup& args);

Type* getIntegerType(int bytes, bool isSigned);

void createBuiltinTypes();

extern vector<Type*> primitives;
extern map<string, Type*> primNames;

typedef DeferredLookup<Type, Type* (*)(TypeLookup&), TypeLookup, string (*)(TypeLookup&)> DeferredTypeLookup;
//global type lookup to be used by some type constructors
extern DeferredTypeLookup* typeLookup;

typedef DeferredLookup<Trait, Trait* (*)(TraitLookup&), TraitLookup, string (*)(TraitLookup&)> DeferredTraitLookup;
extern DeferredTraitLookup* traitLookup;

struct Type
{
  Type(Scope* enclosingScope);
  //list of primitive Types corresponding 1-1 with TypeNT::Prim values
  Scope* enclosing;
  //lazily create/return array type for given number of dims (and all fewer)
  //overridden by ArrayType
  virtual ArrayType* getArrayType(int dims);
  //get integer type corresponding to given size (bytes) and signedness
  virtual bool canConvert(Type* other) = 0;
  virtual bool canConvert(Expression* other);
  //get the type's name
  virtual string getName() = 0;
  //"primitives" maps TypeNT::Prim values to corresponding Type*
  virtual bool implementsTrait(Trait* t) {return false;}
  virtual bool isArray()    {return false;}
  virtual bool isStruct()   {return false;}
  virtual bool isUnion()    {return false;}
  virtual bool isMap()      {return false;}
  virtual bool isTuple()    {return false;}
  virtual bool isEnum()     {return false;}
  virtual bool isCallable() {return false;}
  virtual bool isProc()     {return false;}
  virtual bool isFunc()     {return false;}
  virtual bool isInteger()  {return false;}
  virtual bool isNumber()   {return false;}
  virtual bool isFloat()    {return false;}
  virtual bool isChar()     {return false;}
  virtual bool isBool()     {return false;}
  virtual bool isVoid()     {return false;}
  virtual bool isPrimitive(){return false;}
  virtual bool isAlias()    {return false;}
  virtual bool isBounded()  {return false;}
};

//Bounded type: a set of traits that define a polymorphic argument type (like Java)
//Only used in subroutine declarations, and belongs to subroutine scope
struct BoundedType : public Type
{
  BoundedType(Parser::BoundedTypeNT* nt, Scope* s);
  BoundedType(string n, vector<Trait*> t, Scope* s) : Type(s), name(n), traits(t) {}
  string name;
  vector<Trait*> traits;
  bool canConvert(Type* other)
  {
    return other == this;
  }
  bool canConvert(Expression* other);
  bool isBounded()
  {
    return true;
  }
  string getName()
  {
    return name;
  }
};

struct Trait
{
  Trait(Parser::TraitDecl* td, TraitScope* parent);
  string name;
  vector<string> subrNames;
  vector<CallableType*> callables;
};

struct StructType : public Type
{
  StructType(Parser::StructDecl* sd, Scope* enclosingScope, StructScope* structScope);
  string name;
  vector<Variable*> members;
  vector<bool> composed;      //1-1 correspondence with members
  vector<Trait*> traits;
  //members are variables in structScope or some child of it
  StructScope* structScope;
  bool canConvert(Type* other);
  bool canConvert(Expression* other);
  bool isStruct() {return true;}
  bool implementsTrait(Trait* t);
  void checkTraits(); //called once at end of semantic checking
  string getName()
  {
    return name;
  }
};

struct UnionType : public Type
{
  UnionType(vector<Type*> types);
  vector<Type*> options;
  bool canConvert(Type* other);
  bool isUnion() {return true;}
  string getName();
};

struct UnionCompare
{
  bool operator()(const UnionType* lhs, const UnionType* rhs);
};

struct ArrayType : public Type
{
  ArrayType(Type* elemType, int dims);
  //Type of single element (0-dimensional)
  Type* elem;
  //Type of element of this array type (can be (dims-1) dimensional array, or same as elem)
  Type* subtype;
  Parser::TypeNT* elemNT;
  int dims;
  ArrayType* getArrayType(int extradims);
  bool canConvert(Type* other);
  bool canConvert(Expression* other);
  bool isArray() {return true;}
  string getName()
  {
    string name = elem->getName();
    for(int i = 0; i < dims; i++)
    {
      name += "[]";
    }
    return name;
  }
};

struct ArrayCompare
{
  bool operator()(const ArrayType* lhs, const ArrayType* rhs);
};

struct TupleType : public Type
{
  //TupleType has no scope, so ctor doesn't need it
  TupleType(vector<Type*> members);
  vector<Type*> members;
  bool canConvert(Type* other);
  bool canConvert(Expression* other);
  bool isTuple() {return true;}
  string getName()
  {
    string name = "(";
    for(size_t i = 0; i < members.size(); i++)
    {
      name += members[i]->getName();
      if(i != members.size() - 1)
      {
        name += ", ";
      }
    }
    name += ")";
    return name;
  }
};

struct TupleCompare
{
  bool operator()(const TupleType* lhs, const TupleType* rhs);
};

struct MapType : public Type
{
  MapType(Type* k, Type* v) : Type(NULL), key(k), value(v) {}
  Type* key;
  Type* value;
  bool isMap() {return true;}
  string getName()
  {
    string name = "(";
    name += key->getName();
    name += ", ";
    name += value->getName();
    name += ")";
    return name;
  }
};

struct MapCompare
{
  bool operator()(const MapType* lhs, const MapType* rhs);
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
  bool isArray()    {return actual->isArray();}
  bool isStruct()   {return actual->isStruct();}
  bool isUnion()    {return actual->isUnion();}
  bool isTuple()    {return actual->isTuple();}
  bool isMap()      {return actual->isMap();}
  bool isEnum()     {return actual->isEnum();}
  bool isCallable() {return actual->isCallable();}
  bool isProc()     {return actual->isProc();}
  bool isFunc()     {return actual->isFunc();}
  bool isInteger()  {return actual->isInteger();}
  bool isNumber()   {return actual->isNumber();}
  bool isBool()     {return actual->isBool();}
  bool isVoid()     {return actual->isVoid();}
  bool isPrimitive(){return actual->isPrimitive();}
  bool isAlias()    {return true;}
  string getName()
  {
    return name;
  }
};

struct EnumType : public Type
{
  EnumType(Parser::Enum* e, Scope* enclosingScope);
  string name;
  map<string, int64_t> values;
  int bytes;    //number of bytes required to store all possible values (signed)
  bool canConvert(Type* other);
  //Enum values are equivalent to plain "int"s
  bool isEnum() {return true;}
  bool isInteger() {return true;}
  bool isNumber() {return true;}
  bool isPrimitive() {return true;}
  string getName()
  {
    return name;
  }
};

struct IntegerType : public Type
{
  IntegerType(string name, int size, bool sign);
  //Size in bytes
  string name;
  int size;
  bool isSigned;
  bool canConvert(Type* other);
  bool isInteger() {return true;}
  bool isNumber() {return true;}
  bool isPrimitive() {return true;}
  string getName()
  {
    return name;
  }
};

struct FloatType : public Type
{
  FloatType(string name, int size);
  string name;
  //4 or 8 (bytes, not bits)
  int size;
  bool canConvert(Type* other);
  bool isNumber() {return true;}
  bool isPrimitive() {return true;}
  bool isFloat() {return true;};
  string getName()
  {
    return name;
  }
};

struct CharType : public Type
{
  CharType() : Type(nullptr) {}
  bool canConvert(Type* other);
  bool isChar() {return true;}
  bool isPrimitive() {return true;}
  string getName()
  {
    return "char";
  }
};

struct BoolType : public Type
{
  BoolType();
  bool canConvert(Type* other);
  bool isBool() {return true;}
  bool isPrimitive() {return true;}
  string getName()
  {
    return "bool";
  }
};

struct VoidType : public Type
{
  VoidType();
  bool canConvert(Type* other);
  bool isVoid() {return true;}
  bool isPrimitive() {return true;}
  string getName()
  {
    return "void";
  }
};

struct CallableType : public Type
{
  //constructor for non-member callables
  CallableType(bool isPure, Type* returnType, vector<Type*>& args, bool nonterm = false);
  //constructor for members
  CallableType(bool isPure, StructType* owner, Type* returnType, vector<Type*>& args, bool nonterm = false);
  string getName();
  StructType* ownerStruct;  //true iff non-static and in struct scope
  Type* returnType;
  vector<Type*> argTypes;
  bool pure;            //true for functions, false for procedures
  bool nonterminating;
  bool isCallable()
  {
    return true;
  }
  bool isFunc()
  {
    return pure;
  }
  bool isProc()
  {
    return !pure;
  }
  //Conversion rules:
  //all funcs can be procs
  //ownerStructs must match exactly
  //all terminating procedures can be used in place of nonterminating ones
  //argument and owner types must match exactly (except nonmember -> member)
  bool canConvert(Type* other);
  bool canConvert(Expression* other);
};

struct CallableCompare
{
  bool operator()(const CallableType* lhs, const CallableType* rhs);
};

struct TType : public Type
{
  TType(TraitScope* ts) : Type(ts) {}
  //canConvert: other implements this trait
  bool canConvert(Type* other);
  bool canConvert(Expression* other);
  string getName()
  {
    return "T";
  }
};

} //namespace TypeSystem

#endif

