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

struct Scope;
struct Expression;
struct EnumExpr;
struct CallExpr;
struct SimpleConstant;

struct Type : public Node
{
  virtual ~Type() {}
  virtual bool canConvert(Type* other) = 0;
  //get the type's name
  virtual string getName() const = 0;
  //for types, resolve is only used to detect circular membership
  //i.e. a struct cannot contain itself (must have fixed, finite size)
  //so resolveImpl is only implemented for types where self-membership is
  //not allowed
  virtual void resolveImpl() {}
  //Semantic information
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
  virtual bool isSimple()   {return false;}
  virtual bool isRecursive() {return false;}
  virtual size_t hash() const = 0;
  //Get a constant expression representing the "default"
  //or uninitialized value for the type, usable for constant folding etc.
  virtual Expression* getDefaultValue()
  {
    cout << "Need to implement getDefaultValue() for " << getName() << "!\n";
    INTERNAL_ERROR;
    return nullptr;
  }
  //get the set of types which may be "owned" as a member by this type
  //default is just this type (only valid for the non-compound types)
  virtual void dependencies(set<Type*>& types)
  {
    types.insert(this);
  }
};

/* ********************* */
/* Common Type utilities */
/* ********************* */

//If t is an unresolved type, replace it with a fully resolved version
//(if possible)
void resolveType(Type*& t);

//Check if the types are semantically equivalent
bool typesSame(const Type* t1, const Type* t2);

//Remove all alias wrappers around a type
Type* canonicalize(Type* t);

/* *************** */
/* Primitive types */
/* *************** */

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

IntegerType* getIntegerType(int bytes, bool isSigned);

//Recursive function to generate arbitrary-dimension array type
//if elem is already an array type, will generate array with dimensions = ndims + elem->dims
//if ndims is 0, just returns elem
Type* getArrayType(Type* elem, int ndims);
Type* getTupleType(vector<Type*>& members);
Type* getUnionType(vector<Type*>& options);
Type* getMapType(Type* key, Type* value);
Type* getSubroutineType(StructType* owner, bool pure, Type* returnValue, vector<Type*>& paramTypes);

//If lhs and rhs are both numbers, return the best type for the result
//If either is not a number, NULL
Type* promote(Type* lhs, Type* rhs);

//get (t | void)
Type* maybe(Type* t);

void createBuiltinTypes();

extern map<string, Type*> primNames;

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
  string getName() const
  {
    return name;
  }
  //Given a sequence of names, build a resolved StructMem/SubroutineOverloadExpr
  Expression* findMember(Expression* thisExpr, string* names, size_t numNames, size_t& namesUsed);
  size_t hash() const
  {
    //structs are pointer-unique
    return fnv1a(this);
  }
  void resolveImpl();
  Expression* getDefaultValue();
  void dependencies(set<Type*>& types);
};

struct UnionType : public Type
{
  UnionType(vector<Type*> types);
  void resolveImpl();
  vector<Type*> options;
  vector<size_t> optionHashes;
  bool canConvert(Type* other);
  bool isUnion() {return true;}
  bool isRecursive() {return recursive;}
  string getName() const;
  void dependencies(set<Type*>& types);
  size_t hash() const
  {
    if(recursive)
      return fnv1a(this);
    FNV1A f;
    for(auto t : options)
    {
      f.pump(t->hash());
    }
    return f.get();
  }
  int getTypeIndex(Type* option);
  //defaultVal is precomputed unlike all other types,
  //since it can be somewhat expensive to compute for
  //deeply recursive unions
  Expression* defaultVal;
  //Does the union possibly contain itself?
  //If yes, it will be stored as a void* with tag.
  //If no, it will be stored as C union with tag.
  //  No indirection to access, and no heap storage.
  //  All on stack/static, so much faster, simpler to manage.
  bool recursive;
  //Lazily create and return defaultVal
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
  void dependencies(set<Type*>& types);
  string getName() const
  {
    string name = subtype->getName();
    name += "[]";
    return name;
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(dims);
    f.pump(elem->hash());
    return f.get();
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
  void dependencies(set<Type*>& types);
  string getName() const
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
  size_t hash() const
  {
    FNV1A f;
    for(auto m : members)
      f.pump(m->hash());
    return f.get();
  }
  Expression* getDefaultValue();
};

struct MapType : public Type
{
  MapType(Type* k, Type* v);
  Type* key;
  Type* value;
  bool isMap() {return true;}
  void dependencies(set<Type*>& types);
  string getName() const
  {
    string name = "(";
    name += key->getName();
    name += ", ";
    name += value->getName();
    name += ")";
    return name;
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(3 * key->hash());
    f.pump(value->hash());
    return f.get();
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
  string getName() const
  {
    return name;
  }
  void dependencies(set<Type*>& types)
  {
    actual->dependencies(types);
  }
  bool canConvert(Type* other)
  {
    return actual->canConvert(other);
  }
  Expression* getDefaultValue()
  {
    return actual->getDefaultValue();
  }
  size_t hash() const
  {
    return actual->hash();
  }
};

struct EnumConstant : public Node
{
  //use this constructor for all non-negative values
  EnumConstant(string n, uint64_t u, bool sign = false)
  {
    name = n;
    isSigned = sign;
    value = u;
  }
  EnumType* et;
  string name;
  bool isSigned;  //should value be interpreted as int64_t?
  uint64_t value;
};

struct EnumType : public Type
{
  EnumType(string name, Scope* enclosingScope);
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
  Expression* getDefaultValue();
  //The type used to represent the enum in memory -
  //is able to represent every value
  IntegerType* underlying;
  Scope* scope;
  string getName() const
  {
    return name;
  }
  size_t hash() const
  {
    return fnv1a(this);
  }
private:
  //singleton default value
  EnumExpr* defVal;
};

struct IntegerType : public Type
{
  //size is in bytes (1, 2, 4, 8)
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
  string getName() const
  {
    return name;
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(size);
    f.pump(isSigned);
    return f.get();
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
  string getName() const
  {
    return name;
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(size);
    return f.get();
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
  string getName() const
  {
    return "char";
  }
  size_t hash() const
  {
    return fnv1a((char) 1);
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
  string getName() const
  {
    return "bool";
  }
  size_t hash() const
  {
    return fnv1a((char) 2);
  }
  Expression* getDefaultValue();
};

struct SimpleType : public Type
{
  SimpleType(string n);
  bool canConvert(Type* other)
  {
    other = canonicalize(other);
    return other == this;
  }
  //for all purposes, this is POD and primitive
  bool isPrimitive() {return true;}
  bool isSimple() {return true;}
  string getName() const
  {
    return name;
  }
  size_t hash() const
  {
    return fnv1a(this);
  }
  Expression* getDefaultValue();
  SimpleConstant* val;
  string name;
};

//Type of a specific callable:
//purity, 'this' type, return type and parameter types
struct CallableType : public Type
{
  //constructor for non-member callables
  CallableType(bool isPure, Type* returnType, vector<Type*>& params);
  //constructor for members
  CallableType(bool isPure, StructType* owner, Type* returnType, vector<Type*>& params);
  virtual void resolveImpl();
  string getName() const;
  StructType* ownerStruct;  //true iff non-static and in struct scope
  Type* returnType;
  vector<Type*> paramTypes;
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
  //parameter and owner types must match exactly (except nonmember -> member)
  bool canConvert(Type* other);
  Expression* getDefaultValue()
  {
    INTERNAL_ERROR;
    return nullptr;
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(pure);
    f.pump(returnType->hash());
    f.pump(ownerStruct);
    for(auto p : paramTypes)
    {
      f.pump(p->hash());
    }
    return f.get();
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
  struct Union
  {
    Union(vector<Type*>& m) : members(m) {}
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
  variant<Prim::PrimType, Member*, Tuple, Union, Map, Callable> t;
  Scope* scope;
  int arrayDims;
  //UnresolvedType can never be resolved; it is replaced by something else
  bool canConvert(Type* other) {return false;}
  virtual string getName() const {return "<UNKNOWN TYPE>";}
  size_t hash() const {INTERNAL_ERROR; return 0;}
};

//The type of an unresolved expression
//(used internally for array for loops (and TODO auto vars))
struct ExprType : public Type
{
  ExprType(Expression* e);
  void resolveImpl();
  Expression* expr;
  bool canConvert(Type* other) {return false;}
  string getName() const {return "<unresolved expression type>";};
  size_t hash() const {INTERNAL_ERROR; return 0;}
};

//Used by for-over-array to create an iteration variable at parse time
//passing this to resolveType replaces it by arr's element type
struct ElemExprType : public Type
{
  ElemExprType(Expression* arr);
  ElemExprType(Expression* arr, int reduction);
  void resolveImpl();
  Expression* arr;
  //how many array dimensions to remove from arr
  int reduction;
  bool canConvert(Type* other) {return false;}
  string getName() const {return "<unresolved array expr element type>";};
  size_t hash() const {INTERNAL_ERROR; return 0;}
};

struct TypeEqual
{
  bool operator()(const Type* t1, const Type* t2) const
  {
    if(t1 == t2)
      return true;
    return typesSame(t1, t2);
  }
};

struct TypeHash
{
  size_t operator()(const Type* t) const
  {
    return t->hash();
  }
};

#endif

