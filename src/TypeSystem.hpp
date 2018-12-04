#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include "Parser.hpp"
#include "Scope.hpp"
#include "TypeSystem.hpp"
#include <limits>

using std::numeric_limits;

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

struct Type;
struct StructType;
struct ArrayType;
struct TupleType;
struct UnionType;
struct MapType;
struct AliasType;
struct IntegerType;
struct FloatType;
struct CharType;
struct BoolType;
struct CallableType;
struct EnumType;
//The general unary type
//"void" and "error" are built-in types of this kind
struct SimpleType;
struct UnresolvedType;
struct ExprType;
struct ElemExprType;

struct SimpleConstant;

struct Type : public Node
{
  virtual ~Type() {}
  virtual bool canConvert(Type* other) = 0;
  //get the type's name
  virtual string getName() = 0;
  //for types, resolve is only used to detect circular membership
  //i.e. a struct cannot contain itself (must have fixed, finite size)
  //so resolveImpl is only implemented for types where self-membership is
  //not allowed
  virtual void resolveImpl() {}
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
  virtual bool isPrimitive(){return false;}
  virtual bool isAlias()    {return false;}
  virtual bool isSimple()    {return false;}
  virtual bool isResolved() {return true;}
  //Get a constant expression representing the "default"
  //or uninitialized value for the type, usable for constant folding etc.
  virtual Expression* getDefaultValue()
  {
    cout << "Need to implement getDefaultValue() for " << getName() << "!\n";
    INTERNAL_ERROR;
    return nullptr;
  }
  //get the set of types which may be "owned" as a member by this type
  set<Type*> dependencies()
  {
    vector<UnionType*> empty;
    return dependencies(empty);
  }
  //In order to respect the fact that unions can own themselves directly or indirectly,
  //need to keep track of which unions have already been accounted for (avoid infinite recursion)
  virtual set<Type*> dependencies(vector<UnionType*>& exclude)
  {
    //for all non-compound types, only dependency is itself
    set<Type*> s;
    s.insert(this);
    return s;
  }
};

struct Scope;
struct Expression;

namespace Prim
{
  enum PrimType
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
}

extern vector<Type*> primitives;

struct ArrayCompare
{
  bool operator()(const ArrayType* lhs, const ArrayType* rhs) const;
};

struct TupleCompare
{
  bool operator()(const TupleType* lhs, const TupleType* rhs) const;
};

struct UnionCompare
{
  bool operator()(const UnionType* lhs, const UnionType* rhs) const;
};

struct MapCompare
{
  bool operator()(const MapType* lhs, const MapType* rhs) const;
};

struct CallableType;

struct CallableCompare
{
  bool operator()(const CallableType* lhs, const CallableType* rhs) const;
};

IntegerType* getIntegerType(int bytes, bool isSigned);

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

//get (t | void)
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
  struct IfaceMember
  {
    IfaceMember() : member(nullptr)
    {
      callable = (Subroutine*) nullptr;
    }
    IfaceMember(Variable* m, Subroutine* s) : member(m), callable(s) {}
    IfaceMember(Variable* m, Variable* v) : member(m), callable(v) {}
    Variable* member; //the composed member, or NULL for this
    variant<Subroutine*, Variable*> callable;
  };
  map<string, IfaceMember> interface;
  void resolveImpl();
  Expression* getDefaultValue();
  set<Type*> dependencies(vector<UnionType*>& exclude);
};

struct UnionType : public Type
{
  UnionType(vector<Type*> types);
  void resolveImpl();
  vector<Type*> options;
  bool canConvert(Type* other);
  bool isUnion() {return true;}
  void setDefault();
  string getName();
  set<Type*> dependencies(vector<UnionType*>& exclude);
  //the default type (the first type that doesn't cause infinite recursion)
  int defaultType;
  //A union needs a "short name" so that getName() for
  //self-containing unions returns a finite string
  //All self-containing unions must be the underlying type of some AliasType,
  //so a short name will always be available
  bool hasShortName;
  string shortName;
  Expression* getDefaultValue();
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
  void resolveImpl();
  bool isArray() {return true;}
  set<Type*> dependencies(vector<UnionType*>& exclude);
  string getName()
  {
    string name = elem->getName();
    for(int i = 0; i < dims; i++)
    {
      name += "[]";
    }
    return name;
  }
  Expression* getDefaultValue();
};

struct TupleType : public Type
{
  //TupleType has no scope, so ctor doesn't need it
  TupleType(vector<Type*> members);
  ~TupleType() {}
  vector<Type*> members;
  bool canConvert(Type* other);
  void resolveImpl();
  bool isTuple() {return true;}
  set<Type*> dependencies(vector<UnionType*>& exclude);
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
  Expression* getDefaultValue();
};

struct MapType : public Type
{
  MapType(Type* k, Type* v);
  Type* key;
  Type* value;
  bool isMap() {return true;}
  set<Type*> dependencies(vector<UnionType*>& exclude);
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
  void resolveImpl();
};

struct AliasType : public Type
{
  AliasType(string alias, Type* underlying, Scope* scope);
  void resolveImpl();
  string name;
  Type* actual;
  Scope* scope;
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
  bool isPrimitive(){return actual->isPrimitive();}
  bool isAlias()    {return true;}
  string getName()
  {
    return name;
  }
  set<Type*> dependencies(vector<UnionType*>& exclude)
  {
    return actual->dependencies(exclude);
  }
};

struct EnumConstant : public Node
{
  //use this constructor for all non-negative values
  EnumConstant(string n, uint64_t u)
  {
    name = n;
    fitsS64 = true;
    fitsU64 = true;
    if(u > numeric_limits<int64_t>::max())
    {
      fitsS64 = false;
    }
    else
    {
      sval = u;
    }
    uval = u;
  }
  EnumConstant(string n, int64_t s)
  {
    name = n;
    fitsS64 = true;
    fitsU64 = false;
    sval = s;
  }
  EnumType* et;
  string name;
  uint64_t uval;
  int64_t sval;
  bool fitsS64;
  bool fitsU64;
};

struct EnumType : public Type
{
  EnumType(Scope* enclosingScope);
  //resolving an enum decides what its underlying type should be
  void resolveImpl();
  //add a name to enum, automatically choosing numeric value
  //as the smallest value greater than the most recently
  //added value, which is not already in the enum
  void addAutomaticValue(string name, Node* location);
  //add a name with user-provided unsigned (non-negative) value
  void addPositiveValue(string name, uint64_t val, Node* location);
  void addNegativeValue(string name, int64_t val, Node* location);
  string name;
  vector<EnumConstant*> values;
  bool canConvert(Type* other);
  //Enum values are equivalent to plain "int"s
  bool isEnum() {return true;}
  bool isInteger() {return true;}
  bool isNumber() {return true;}
  //The type used to represent the enum in memory
  IntegerType* underlying;
  Scope* scope;
  string getName()
  {
    return name;
  }
};

struct IntegerType : public Type
{
  IntegerType(string name, int size, bool sign);
  uint64_t maxUnsignedVal();
  int64_t minSignedVal();
  int64_t maxSignedVal();
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
  Expression* getDefaultValue();
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
  Expression* getDefaultValue();
};

struct CharType : public Type
{
  CharType()
  {
    resolved = true;
  }
  bool canConvert(Type* other);
  bool isChar() {return true;}
  bool isInteger() {return true;}
  bool isNumber() {return true;}
  bool isPrimitive() {return true;}
  string getName()
  {
    return "char";
  }
  Expression* getDefaultValue();
};

struct BoolType : public Type
{
  BoolType()
  {
    resolved = true;
  }
  bool canConvert(Type* other);
  bool isBool() {return true;}
  bool isPrimitive() {return true;}
  string getName()
  {
    return "bool";
  }
  Expression* getDefaultValue();
};

struct SimpleType : public Type
{
  SimpleType(string n);
  bool canConvert(Type* other)
  {
    return other == this;
  }
  //for all purposes, this is POD and primitive
  bool isPrimitive() {return true;}
  string getName()
  {
    return name;
  }
  Expression* getDefaultValue();
  SimpleConstant* val;
  string name;
};

struct CallableType : public Type
{
  //constructor for non-member callables
  CallableType(bool isPure, Type* returnType, vector<Type*>& args);
  //constructor for members
  CallableType(bool isPure, StructType* owner, Type* returnType, vector<Type*>& args);
  virtual void resolveImpl();
  string getName();
  StructType* ownerStruct;  //true iff non-static and in struct scope
  Type* returnType;
  vector<Type*> argTypes;
  bool pure;            //true for functions, false for procedures
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
  Expression* getDefaultValue()
  {
    INTERNAL_ERROR;
    return nullptr;
  }
};

struct UnresolvedType : public Type
{
  //tuple and union are both just vectors of types, so need this
  //to differentiate them in the variant
  struct Tuple
  {
    Tuple(vector<Type*>& m) : members(m) {}
    vector<Type*> members;
  };
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
  //Note that UnionType is one of the options here
  //UnionType is allowed to have unresolved members, and its resolveImpl()
  //is needed to gracefully deal with circular dependencies (impossible in
  //resolveType())
  variant<Prim::PrimType, Member*, Tuple, UnionType*, Map, Callable> t;
  Scope* scope;
  int arrayDims;
  //UnresolvedType can never be resolved; it is replaced by something else
  bool isResolved() {return false;}
  bool canConvert(Type* other) {return false;}
  virtual string getName() {return "<UNKNOWN TYPE>";}
};

//The type of an unresolved expression
//(used internally for array for loops (and TODO auto vars))
struct ExprType : public Type
{
  ExprType(Expression* e);
  void resolveImpl();
  Expression* expr;
  bool isResolved() {return false;}
  bool canConvert(Type* other) {return false;}
  string getName() {return "<unresolved expression type>";};
};

//Used by for-over-array to create an iteration variable at parse time
//passing this to resolveType replaces it by arr's element type
struct ElemExprType : public Type
{
  ElemExprType(Expression* arr);
  void resolveImpl();
  Expression* arr;
  bool isResolved() {return false;}
  bool canConvert(Type* other) {return false;}
  string getName() {return "<unresolved array expr element type>";};
};

//If t is an unresolved type, replace it with a fully resolved version
//(if possible)
void resolveType(Type*& t);

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

