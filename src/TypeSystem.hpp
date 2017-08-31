#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include "Parser.hpp"
#include "Scope.hpp"
#include "TypeSystem.hpp"
#include "AST_Printer.hpp"
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

struct TypeLookup
{
  TypeLookup(Parser::TypeNT* t, Scope* s) : type(t), scope(s) {}
  Parser::TypeNT* type;
  Scope* scope;
};

//type error message function, to be used by DeferredLookup on types
string typeErrorMessage(TypeLookup& lookup);

Type* lookupType(Parser::TypeNT* type, Scope* scope);
Type* lookupTypeDeferred(TypeLookup& args);

Type* getIntegerType(int bytes, bool isSigned);

void createBuiltinTypes();

extern vector<TupleType*> tuples;
extern vector<Type*> primitives;
extern map<string, Type*> primNames;

typedef DeferredLookup<Type, Type* (*)(TypeLookup&), TypeLookup, string (*)(TypeLookup&)> DeferredTypeLookup;
//global type lookup to be used by some type constructors
extern DeferredTypeLookup* typeLookup;

struct Type
{
  Type(Scope* enclosingScope);
  //list of primitive Types corresponding 1-1 with TypeNT::Prim values
  Scope* enclosing;
  //dimTypes[0] is for T[], dimTypes[1] is for T[][], etc.
  vector<Type*> dimTypes;
  // [lazily create and] return array type for given number of dimensions of *this
  Type* getArrayType(int dims);
  //get integer type corresponding to given size (bytes) and signedness
  virtual bool canConvert(Type* other) = 0;
  virtual bool canConvert(Expression* other);
  //get the type's name
  virtual string getName() = 0;
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
  virtual bool isVoid()     {return false;}
  virtual bool isPrimitive(){return false;}
  virtual bool isAlias()    {return false;}
};

/*
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
*/

struct StructType : public Type
{
  StructType(Parser::StructDecl* sd, Scope* enclosingScope, StructScope* structScope);
  string name;
  //check for member functions
  //note: self doesn't count as an argument but it is the 1st arg internally
  bool hasFunc(FuncType* type);
  bool hasProc(ProcType* type);
  //vector<Trait*> traits;
  vector<Type*> members;
  vector<string> memberNames; //1-1 correspondence with members
  vector<bool> composed;      //1-1 correspondence with members
  //used to handle unresolved data members
  Parser::StructDecl* decl;
  //member types must be searched from here (the scope inside the struct decl)
  StructScope* structScope;
  bool canConvert(Type* other);
  bool canConvert(Expression* other);
  bool isStruct();
  string getName()
  {
    return name;
  }
};

struct UnionType : public Type
{
  UnionType(Parser::UnionDecl* ud, Scope* enclosingScope);
  string name;
  vector<Type*> options;
  Parser::UnionDecl* decl;
  bool canConvert(Type* other);
  bool isUnion();
  string getName()
  {
    return name;
  }
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

struct TupleType : public Type
{
  //TupleType has no scope, so ctor doesn't need it
  TupleType(vector<Type*> members);
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
  bool isEnum()     {return actual->isEnum();}
  bool isCallable() {return actual->isCallable();}
  bool isProc()     {return actual->isProc();}
  bool isFunc()     {return actual->isFunc();}
  bool isInteger()  {return actual->isInteger();}
  bool isNumber()   {return actual->isNumber();}
  bool isString()   {return actual->isString();}
  bool isBool()     {return actual->isBool();}
  bool isConcrete() {return actual->isConcrete();}
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
  map<string, int> values;
  bool canConvert(Type* other);
  //Enum values are equivalent to plain "int"s
  bool isEnum();
  bool isInteger();
  bool isNumber();
  bool isPrimitive();
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
  bool isInteger();
  bool isNumber();
  bool isPrimitive();
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
  bool isNumber();
  bool isPrimitive();
  string getName()
  {
    return name;
  }
};

struct StringType : public Type
{
  StringType();
  bool canConvert(Type* other);
  bool isString();
  bool isPrimitive();
  string getName()
  {
    return "string";
  }
};

struct BoolType : public Type
{
  BoolType();
  bool canConvert(Type* other);
  bool isBool();
  bool isPrimitive();
  string getName()
  {
    return "bool";
  }
};

struct VoidType : public Type
{
  VoidType();
  bool canConvert(Type* other);
  bool isVoid();
  bool isPrimitive();
  string getName()
  {
    return "void";
  }
};

/*
struct TType : public Type
{
  TType();
  static TType inst;
  bool canConvert(Type* other);
};
*/

} //namespace TypeSystem

#endif

