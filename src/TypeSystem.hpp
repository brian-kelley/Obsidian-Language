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
struct Expression;

enum struct Prim
{
  BOOL,
  CHAR,
  BYTE,
  UBYTE,
  SHORT,
  USHORT,
  INT,
  UINT,
  LONG,
  ULONG,
  FLOAT,
  DOUBLE,
  VOID,
  ERROR
};

extern vector<Type*> primitives;

namespace TypeSystem
{

struct Type;
//All Type subclasses:
struct StructType;
struct TupleType;
struct ArrayType;
struct UnionType;
struct MapType;
struct AliasType;
struct EnumType;
struct IntegerType;
struct FloatType;
struct CharType;
struct BoolType;
struct VoidType;
struct TType;
struct CallableType;
struct ErrorType;

struct ArrayCompare
{
  bool operator()(const ArrayType* lhs, const ArrayType* rhs);
};

struct TupleCompare
{
  bool operator()(const TupleType* lhs, const TupleType* rhs);
};

struct UnionCompare
{
  bool operator()(const UnionType* lhs, const UnionType* rhs);
};

struct MapCompare
{
  bool operator()(const MapType* lhs, const MapType* rhs);
};

struct CallableType;

struct CallableCompare
{
  bool operator()(const CallableType* lhs, const CallableType* rhs);
};

Type* getIntegerType(int bytes, bool isSigned);

//Recursive function to generate arbitrary-dimension array type
//if elem is already an array type, will generate array with dimensions = ndims + elem->dims
//if ndims is 0, just returns elem
Type* getArrayType(Type* elem, int ndims);
Type* getTupleType(vector<Type*>& members);
Type* getUnionType(vector<Type*>& options);
Type* getMapType(Type* key, Type* value);
Type* getSubroutineType(StructType* owner, bool pure, bool nonterm, Type* returnValue, vector<Type*>& argTypes);

//If lhs and rhs are both numbers, return the best type for the result
//If either is not a number, NULL
Type* promote(Type* lhs, Type* rhs);

//get (t | Error)
Type* maybe(Type* t);

void createBuiltinTypes();

extern map<string, Type*> primNames;

extern vector<StructType*> structs;
extern set<ArrayType*, ArrayCompare> arrays;
extern set<TupleType*, TupleCompare> tuples;
extern set<UnionType*, UnionCompare> unions;
extern set<MapType*, MapCompare> maps;
extern set<CallableType*, CallableCompare> callables;
extern set<EnumType*> enums;

typedef DeferredLookup<Type, Type* (*)(TypeLookup&), TypeLookup, string (*)(TypeLookup&)> DeferredTypeLookup;
//global type lookup to be used by some type constructors
extern DeferredTypeLookup* typeLookup;

//Type extends Node, but Node members are only used for types that
//have unique definitions (struct, alias, enum)

struct Type : public Node
{
  virtual ~Type() {}
  virtual bool canConvert(Type* other) = 0;
  //get the type's name
  virtual string getName() = 0;
  //whether this "contains" t, for finding circular memberships
  virtual bool contains(Type* t) {return false;}
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
  virtual bool isResolved() {return true;}
};

struct StructType : public Type
{
  //Constructor just creates an empty struct (no members)
  //Parser should explicitly add members as they are parsed
  StructType(string name, Scope* enclosingScope);
  string name;
  vector<Variable*> members;
  vector<bool> composed; //1-1 correspondence with members
  Scope* scope;
  bool canConvert(Type* other);
  bool isStruct() {return true;}
  string getName()
  {
    return name;
  }
  bool contains(Type* t);
  struct IfaceMember
  {
    IfaceMember() : member(nullptr), subr(nullptr) {}
    IfaceMember(Variable* m, Subroutine* s) : member(m), subr(s) {}
    Variable* member; //the composed member, or NULL for this
    Subroutine* subr;
  };
  map<string, IfaceMember> interface;
  void resolveImpl(bool final);
};

struct UnionType : public Type
{
  UnionType(vector<Type*> types);
  vector<Type*> options;
  bool canConvert(Type* other);
  bool isUnion() {return true;}
  string getName();
};

struct ArrayType : public Type
{
  ArrayType(Type* elemType, int dims);
  //Type of single element (0-dimensional)
  Type* elem;
  //Type of element of this array type (can be (dims-1) dimensional array, or same as elem)
  Type* subtype;
  int dims;
  bool canConvert(Type* other);
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
  bool contains(Type* t);
};

struct TupleType : public Type
{
  //TupleType has no scope, so ctor doesn't need it
  TupleType(vector<Type*> members);
  ~TupleType() {}
  vector<Type*> members;
  bool canConvert(Type* other);
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
  bool contains(Type* t);
  void check();
};

struct MapType : public Type
{
  MapType(Type* k, Type* v) : key(k), value(v) {}
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
  bool canConvert(Type* other);
  bool contains(Type* t);
  void check();
};


struct AliasType : public Type
{
  AliasType(string alias, Type* underlying);
  string name;
  Type* actual;
  Parser::Typedef* decl;
  bool canConvert(Type* other);
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
  bool contains(Type* t);
};

struct EnumConstant
{
  string name;
  int64_t value;
  EnumType* et;
};

struct EnumType : public Type
{
  EnumType(Parser::Enum* e, Scope* enclosingScope);
  string name;
  vector<EnumConstant*> values;
  bool canConvert(Type* other);
  //Enum values are equivalent to plain "int"s
  bool isEnum() {return true;}
  bool isInteger() {return true;}
  bool isNumber() {return true;}
  string getName()
  {
    return name;
  }
};

struct IntegerType : public Type
{
  IntegerType(string name, int size, bool sign);
  string name;
  //Size in bytes
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
  bool canConvert(Type* other);
  bool isChar() {return true;}
  bool isInteger() {return true;}
  bool isNumber() {return true;}
  bool isPrimitive() {return true;}
  string getName()
  {
    return "char";
  }
};

struct BoolType : public Type
{
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
  bool canConvert(Type* other);
  bool isVoid() {return true;}
  bool isPrimitive() {return true;}
  string getName()
  {
    return "void";
  }
};

struct ErrorType : public Type
{
  bool canConvert(Type* other)
  {
    return other == this;
  }
  bool isPrimitive() {return true;}
  string getName()
  {
    return "Error";
  }
};

struct CallableType : public Type
{
  //constructor for non-member callables
  CallableType(bool isPure, Type* returnType, vector<Type*>& args);
  //constructor for members
  CallableType(bool isPure, StructType* owner, Type* returnType, vector<Type*>& args);
  string getName();
  Struct* ownerStruct;  //true iff non-static and in struct scope
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
};

struct UnresolvedType : public Type, public Node
{
  //tuple and union are both just vectors of types, so need this
  //to differentiate them in the variant
  struct TupleList : public vector<Type*> {}
  struct UnionList : public vector<Type*> {}
  struct Map
  {
    Map(Type* k, Type* v) : key(k), value(v) {}
    Type* key;
    Type* value;
  };
  struct Callable
  {
    Callable(bool p, bool s, Type* ret, vector<Type*> paramList) :
      pure(p), isStatic(s), returnType(ret), params(paramList) {}
    bool pure;
    bool isStatic;
    Type* returnType;
    vector<Type*> params;
  };
  variant<Prim, Member*, TupleList, UnionList, Map, Callable> t;
  Scope* scope;
  int arrayDims;
  //UnresolvedType can never be resolved; it is replaced by something else
  bool isResolved() {return false;}
};

//The type of an unresolved expression
//(used internally for array for loops (and TODO auto vars))
struct ExprType : public Type
{
  ExprType(Expression* e);
  void resolve(bool final);
  Expression* expr;
  bool isResolved() {return false;}
};

//Used by for-over-array to create an iteration variable at parse time
//passing this to resolveType replaces it by arr's element type
struct ElemExprType : public Type
{
  ElemExprType(Expression* arr);
  void resolve(bool final);
  Expression* arr;
  bool isResolved() {return false;}
};

//If t is an unresolved type, replace it with a fully resolved version
//(if possible)
void resolveType(Type*& t, bool final);

} //namespace TypeSystem

#endif

//All supported type conversions:
//  (case 1) -All primitives can be converted to each other trivially
//    -floats/doubles truncated to integer as in C
//    -ints converted to each other as in C
//    -char treated as integer
//    -any number converted to bool with nonzero being true
//  (case 2) -Out = struct: in = struct or tuple
//  (case 3) -Out = array: in = struct, tuple or array
//  (case 4) -Out = map: in = map, array, or tuple
//    -in = (key' : value'): key' must convert to key and value' to value
//    -in = (key, value)[]: use key-value pairs directly
//  (case 2) -Out = tuple: in = struct or tuple

