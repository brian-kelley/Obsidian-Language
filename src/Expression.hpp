#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "AST.hpp"

struct Expression;
/**********************/
/* Parsed Expressions */
/**********************/
//Subclasses of Expression
//Constants/literals
struct IntConstant;
struct FloatConstant;
struct StringConstant;
struct BoolConstant;
struct MapConstant;
struct CompoundLiteral;
struct UnionConstant;
struct SimpleConstant;
//Arithmetic
struct UnaryArith;
struct BinaryArith;
//Data structure manipulations
struct Indexed;
struct NewArray;
struct ArrayLength;
struct AsExpr;
struct IsExpr;
struct CallExpr;
struct VarExpr;
struct Converted;
struct ThisExpr;

//SubroutineExpr stores a pointer

/*******************************/
/* Placeholders for Resolution */
/*******************************/
struct DefaultValueExpr;
struct UnresolvedExpr;

struct Expression : public Node
{
  Expression() : type(nullptr) {}
  virtual void resolveImpl() {}
  Type* type;
  //whether this works as an lvalue
  virtual bool assignable() = 0;
  //whether this is a compile-time constant
  virtual bool constant() const
  {
    return false;
  }
  //get the number of bytes required to store the constant
  virtual int getConstantSize()
  {
    INTERNAL_ERROR;
    return 0;
  }
  //Get the variables read and written by evaluating.
  //If lhs, don't include the "base" variable.
  virtual void getReads(set<Variable*>& vars, bool lhs) {}
  //All lvalues have exactly one "base" variable.
  //Null for all non-lvalues.
  virtual Variable* getWrite()
  {
    return nullptr;
  }
  //Does this change global state?
  //(call a proc)
  virtual bool hasSideEffects()
  {
    return false;
  }
  //Does this read any globals?
  //(call a proc or reference global)
  virtual bool readsGlobals()
  {
    return false;
  }
  //Is it worth doing CSE to avoid computing this?
  virtual bool isComputation()
  {
    return false;
  }
  //Hash this expression (for use in unordered map/set)
  //Only hard requirement: if a == b, then hash(a) == hash(b)
  virtual size_t hash() const = 0;
  virtual bool operator==(const Expression& rhs) const = 0;
  bool operator!=(const Expression& rhs) const
  {
    return !(*this == rhs);
  }
  //Precondition: constant() && rhs.constant()
  //Only to be used for folding relational binary ops.
  //Only implemented for expressions which can be constant,
  //except MapConstant
  virtual bool operator<(const Expression& rhs) const
  {
    INTERNAL_ERROR;
    return false;
  }
  bool operator>(const Expression& rhs) const
  {
    return rhs < *this;
  }
  bool operator<=(const Expression& rhs) const
  {
    return !(rhs < *this);
  }
  bool operator>=(const Expression& rhs) const
  {
    return !(*this < rhs);
  }
  virtual Variable* getRootVariable() {INTERNAL_ERROR;}
  //deep copy (must already be resolved)
  virtual Expression* copy() = 0;
  virtual ostream& print(ostream& os) = 0;
};

inline ostream& operator<<(ostream& os, Expression* expr)
{
  return expr->print(os);
}

struct ExprEqual
{
  bool operator()(const Expression* lhs, const Expression* rhs) const
  {
    if(lhs == rhs)
      return true;
    return *lhs == *rhs;
  }
};

struct ExprHash
{
  size_t operator()(const Expression* e) const
  {
    return e->hash();
  }
};

struct UnaryArith : public Expression
{
  UnaryArith(int op, Expression* expr);
  int op;
  Expression* expr;
  bool assignable()
  {
    return false;
  }
  void resolveImpl();
  bool hasSideEffects()
  {
    return expr->hasSideEffects();
  }
  bool readsGlobals()
  {
    return expr->readsGlobals();
  }
  bool isComputation()
  {
    return true;
  }
  void getReads(set<Variable*>& vars, bool)
  {
    expr->getReads(vars, false);
  }
  Variable* getWrite()
  {
    return expr->getWrite();
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(op);
    f.pump(expr->hash());
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  ostream& print(ostream& os);
  Expression* copy();
};

struct BinaryArith : public Expression
{
  BinaryArith(Expression* lhs, int op, Expression* rhs);
  int op;
  Expression* lhs;
  Expression* rhs;
  void resolveImpl();
  bool assignable()
  {
    return false;
  }
  bool hasSideEffects()
  {
    return lhs->hasSideEffects() || rhs->hasSideEffects();
  }
  bool readsGlobals()
  {
    return lhs->readsGlobals() || rhs->readsGlobals();
  }
  bool isComputation()
  {
    return true;
  }
  void getReads(set<Variable*>& vars, bool)
  {
    lhs->getReads(vars, false);
    rhs->getReads(vars, false);
  }
  bool commutative()
  {
    return operCommutativeTable[op];
  }
  size_t hash() const
  {
    FNV1A f;
    size_t lhsHash = lhs->hash();
    size_t rhsHash = rhs->hash();
    //Make sure that "a op b" and "b op a" hash the same if op is commutative-
    //operator== says these are identical
    if(operCommutativeTable[op] && lhsHash > rhsHash)
      std::swap(lhsHash, rhsHash);
    f.pump(op);
    f.pump(lhsHash);
    f.pump(rhsHash);
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct IntConstant : public Expression
{
  IntConstant()
  {
    uval = 0;
    sval = 0;
    type = primitives[Prim::ULONG];
    resolved = true;
  }
  IntConstant(IntLit* ast)
  {
    //Prefer a signed type to represent positive integer constants
    auto intType = (IntegerType*) primitives[Prim::INT];
    auto longType = (IntegerType*) primitives[Prim::LONG];
    if(ast->val <= (uint64_t) intType->maxSignedVal())
    {
      type = intType;
      sval = ast->val;
    }
    else if(ast->val <= (uint64_t) longType->maxSignedVal())
    {
      type = longType;
      sval = ast->val;
    }
    else
    {
      type = primitives[Prim::ULONG];
      uval = ast->val;
    }
    resolved = true;
  }
  IntConstant(int64_t val)
  {
    sval = val;
    type = primitives[Prim::LONG];
    resolved = true;
  }
  IntConstant(uint64_t val)
  {
    uval = val;
    type = primitives[Prim::ULONG];
    resolved = true;
  }
  IntConstant(int64_t val, Type* t)
  {
    uval = val;
    sval = val;
    type = t;
    resolved = true;
  }
  IntConstant(uint64_t val, Type* t)
  {
    uval = val;
    sval = val;
    type = t;
    resolved = true;
  }
  //Attempt to convert to int/float/enum type
  //Make sure the conversion is valid and show error
  //if this fails
  Expression* convert(Type* t);
  //Return true if value fits in the type
  bool checkValueFits();
  IntConstant* binOp(int op, IntConstant* rhs);
  int64_t sval;
  uint64_t uval;
  bool assignable()
  {
    return false;
  }
  bool constant() const
  {
    return true;
  }
  int getConstantSize()
  {
    return ((IntegerType*) type)->size;
  }
  bool isSigned() const
  {
    return ((IntegerType*) type)->isSigned;
  }
  size_t hash() const
  {
    //This hash ignores underlying type - exprs
    //will need to be compared exactly anyway
    //and false positives are unlikely
    if(((IntegerType*) type)->isSigned)
      return fnv1a(sval);
    return fnv1a(uval);
  }
  bool operator<(const Expression& rhs) const
  {
    const IntConstant& rhsInt = dynamic_cast<const IntConstant&>(rhs);
    if(isSigned())
      return sval < rhsInt.sval;
    else
      return uval < rhsInt.uval;
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct FloatConstant : public Expression
{
  FloatConstant()
  {
    fp = 0;
    dp = 0;
    type = primitives[Prim::DOUBLE];
    resolved = true;
  }
  FloatConstant(FloatLit* ast)
  {
    dp = ast->val;
    type = primitives[Prim::DOUBLE];
    resolved = true;
  }
  FloatConstant(float val)
  {
    fp = val;
    type = primitives[Prim::FLOAT];
    resolved = true;
  }
  FloatConstant(double val)
  {
    dp = val;
    type = primitives[Prim::DOUBLE];
    resolved = true;
  }
  bool isDoublePrec() const
  {
    return typesSame(primitives[Prim::DOUBLE], this->type);
  }
  Expression* convert(Type* t);
  float fp;
  double dp;
  FloatConstant* binOp(int op, FloatConstant* rhs);
  bool assignable()
  {
    return false;
  }
  bool constant() const
  {
    return true;
  }
  bool operator<(const Expression& rhs) const
  {
    const FloatConstant& rhsFloat = dynamic_cast<const FloatConstant&>(rhs);
    if(isDoublePrec())
      return dp < rhsFloat.dp;
    else
      return fp < rhsFloat.fp;
  }
  int getConstantSize()
  {
    return ((FloatType*) type)->size;
  }
  size_t hash() const
  {
    if(isDoublePrec())
      return fnv1a(dp);
    return fnv1a(fp);
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct StringConstant : public Expression
{
  StringConstant(StrLit* ast)
  {
    value = ast->val;
    type = getArrayType(primitives[Prim::CHAR], 1);
    resolveType(type);
    resolved = true;
  }
  StringConstant(string str)
  {
    value = str;
    type = getArrayType(primitives[Prim::CHAR], 1);
    resolveType(type);
    resolved = true;
  }
  string value;
  bool assignable()
  {
    return false;
  }
  bool constant() const
  {
    return true;
  }
  bool operator<(const Expression& rhs) const
  {
    const StringConstant& rhsStr = dynamic_cast<const StringConstant&>(rhs);
    return value < rhsStr.value;
  }
  int getConstantSize()
  {
    return 16 + value.length() + 1;
  }
  size_t hash() const
  {
    return fnv1a(value.c_str(), value.length());
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct CharConstant : public Expression
{
  CharConstant(CharLit* ast)
  {
    value = ast->val;
    type = primitives[Prim::CHAR];
    resolved = true;
  }
  CharConstant(char c)
  {
    value = c;
    type = primitives[Prim::CHAR];
    resolved = true;
  }
  char value;
  bool assignable()
  {
    return false;
  }
  bool constant() const
  {
    return true;
  }
  bool operator<(const Expression& rhs) const
  {
    const CharConstant& rhsChar = dynamic_cast<const CharConstant&>(rhs);
    return value < rhsChar.value;
  }
  size_t hash() const
  {
    return fnv1a(value);
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct BoolConstant : public Expression
{
  BoolConstant(bool v)
  {
    value = v;
    type = primitives[Prim::BOOL];
    resolved = true;
  }
  bool value;
  bool assignable()
  {
    return false;
  }
  bool constant() const
  {
    return true;
  }
  int getConstantSize()
  {
    return 1;
  }
  bool operator<(const Expression& rhs) const
  {
    const BoolConstant& b = dynamic_cast<const BoolConstant&>(rhs);
    return !value && b.value;
  }
  size_t hash() const
  {
    //don't use FNV-1a here since there are only 2 possible values
    if(value)
      return 0x123456789ABCDEF0ULL;
    else
      return ~0x123456789ABCDEF0ULL;
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

//Map constant: hold set of constant key-value pairs
//Relies on operator== and operator< for Expressions
struct MapConstant : public Expression
{
  MapConstant(MapType* mt);
  unordered_map<Expression*, Expression*, ExprHash, ExprEqual> values;
  bool constant() const
  {
    return true;
  }
  int getConstantSize()
  {
    int total = 0;
    for(auto& kv : values)
    {
      total += kv.first->getConstantSize();
      total += kv.second->getConstantSize();
    }
    return total;
  }
  bool assignable()
  {
    return false;
  }
  size_t hash() const
  {
    //the order of key-value pairs in values is NOT deterministic,
    //so use XOR to combine hashes of each key-value pair
    size_t h = 0;
    for(auto& kv : values)
    {
      FNV1A f;
      f.pump(kv.first->hash());
      f.pump(kv.second->hash());
      h ^= f.get();
    }
    return h;
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

//UnionConstant only used in IR/optimization
//expr->type exactly matches exactly one of ut's options
//(which is guaranteed by semantic checking/implicit conversions)
struct UnionConstant : public Expression
{
  UnionConstant(Expression* expr, UnionType* ut);
  bool assignable()
  {
    return false;
  }
  bool constant() const
  {
    return true;
  }
  bool operator<(const Expression& rhs) const
  {
    const UnionConstant& u = dynamic_cast<const UnionConstant&>(rhs);
    if(option < u.option)
      return true;
    else if(option > u.option)
      return false;
    return *value < *u.value;
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(option);
    f.pump(value->hash());
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
  UnionType* unionType;
  Expression* value;
  int option;
};

//CompoundLiteral represents array, struct and tuple literals and constants.
//Its type (by default) is a tuple of the types of each element; this
//implicitly converts to array, struct and other tuples (elementwise).
struct CompoundLiteral : public Expression
{
  CompoundLiteral(vector<Expression*>& mems);
  CompoundLiteral(vector<Expression*>& mems, Type* t);
  void resolveImpl();
  bool assignable()
  {
    return lvalue;
  }
  vector<Expression*> members;
  //(set during resolution): is every member an lvalue?
  bool lvalue;
  bool constant() const
  {
    for(auto m : members)
    {
      if(!m->constant())
        return false;
    }
    return true;
  }
  int getConstantSize()
  {
    int total = 0;
    for(auto mem : members)
    {
      total += sizeof(void*) + mem->getConstantSize();
    }
    return total;
  }
  bool hasSideEffects()
  {
    for(auto m : members)
    {
      if(m->hasSideEffects())
        return true;
    }
    return false;
  }
  void getReads(set<Variable*>& vars, bool lhs)
  {
    //IR should never contain assignment to CompoundLiteral
    INTERNAL_ASSERT(!lhs);
    for(auto m : members)
    {
      m->getReads(vars, false);
    }
  }
  bool readsGlobals()
  {
    for(auto m : members)
    {
      if(m->readsGlobals())
        return true;
    }
    return false;
  }
  bool operator<(const Expression& rhs) const
  {
    const CompoundLiteral& cl = dynamic_cast<const CompoundLiteral&>(rhs);
    //lex compare
    size_t n = std::min(members.size(), cl.members.size());
    for(size_t i = 0; i < n; i++)
    {
      if(*members[i] < *cl.members[i])
        return true;
      else if(*members[i] != *cl.members[i])
        return false;
    }
    return n == members.size();
  }
  size_t hash() const
  {
    FNV1A f;
    for(auto m : members)
      f.pump(31 * m->hash());
    return f.get();
  }
  bool isComputation()
  {
    for(auto m : members)
    {
      if(m->isComputation())
        return true;
    }
    return false;
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct Indexed : public Expression
{
  Indexed(Expression* grp, Expression* ind);
  void resolveImpl();
  Expression* group; //the array or tuple being subscripted
  Expression* index;
  bool assignable()
  {
    return group->assignable();
  }
  Variable* getRootVariable()
  {
    return group->getRootVariable();
  }
  bool hasSideEffects()
  {
    return group->hasSideEffects() || index->hasSideEffects();
  }
  bool readsGlobals()
  {
    return group->readsGlobals() || index->readsGlobals();
  }
  bool isComputation()
  {
    return true;
  }
  void getReads(set<Variable*>& vars, bool lhs)
  {
    group->getReads(vars, lhs);
    index->getReads(vars, false);
  }
  Variable* getWrite()
  {
    return group->getWrite();
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(7 * group->hash());
    f.pump(index->hash());
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct CallExpr : public Expression
{
  CallExpr(Expression* callable, vector<Expression*>& args);
  void resolveImpl();
  Expression* callable;
  vector<Expression*> args;
  bool assignable()
  {
    return false;
  }
  bool isProc()
  {
    return ((CallableType*) callable->type)->isProc();
  }
  bool hasSideEffects()
  {
    //All procs are assumed to have side effects
    if(isProc())
      return true;
    //The callable expr itself may also have side effects
    if(callable->hasSideEffects())
      return true;
    for(auto a : args)
    {
      if(a->hasSideEffects())
        return true;
    }
    return false;
  }
  bool readsGlobals()
  {
    //All procs are assumed to read globals
    if(isProc())
      return true;
    if(callable->readsGlobals())
      return true;
    for(auto a : args)
    {
      if(a->readsGlobals())
        return true;
    }
    return false;
  }
  bool isComputation()
  {
    return true;
  }
  void getReads(set<Variable*>& vars, bool lhs)
  {
    callable->getReads(vars, false);
    for(auto a : args)
      a->getReads(vars, false);
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(callable->hash());
    for(auto a : args)
      f.pump(a->hash());
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct VarExpr : public Expression
{
  VarExpr(Variable* v, Scope* s);
  VarExpr(Variable* v);
  void resolveImpl();
  Variable* var;  //var must be looked up from current scope
  Scope* scope;
  bool assignable()
  {
    //all variables are lvalues
    return true;
  }
  Variable* getRootVariable()
  {
    return var;
  }
  bool hasSideEffects()
  {
    return false;
  }
  void getReads(set<Variable*>& vars, bool lhs)
  {
    if(!lhs)
      vars.insert(var);
  }
  Variable* getWrite()
  {
    return var;
  }
  bool readsGlobals();
  size_t hash() const
  {
    //variables are uniquely identifiable by pointer
    return fnv1a(var);
  }
  bool operator==(const Expression& erhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

//Expression to represent constant callable
//May be standalone, or may be applied to an object
struct SubroutineExpr : public Expression
{
  SubroutineExpr(Subroutine* s);
  SubroutineExpr(Expression* thisObj, Subroutine* s);
  SubroutineExpr(ExternalSubroutine* es);
  void resolveImpl();
  bool assignable()
  {
    return false;
  }
  bool constant() const
  {
    return true;
  }
  bool hasSideEffects()
  {
    //example, if "a" is a proc,
    //a().callMember() has side effects
    return thisObject && thisObject->hasSideEffects();
  }
  bool readsGlobals()
  {
    return thisObject && thisObject->readsGlobals();
  }
  void getReads(set<Variable*>& vars, bool lhs)
  {
    if(thisObject)
      thisObject->getReads(vars, false);
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(thisObject);
    f.pump(subr);
    f.pump(exSubr);
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
  Subroutine* subr;
  ExternalSubroutine* exSubr;
  Expression* thisObject; //null for static/extern
};

struct StructMem : public Expression
{
  StructMem(Expression* base, Variable* var);
  StructMem(Expression* base, Subroutine* subr);
  void resolveImpl();
  Expression* base;           //base->type is always StructType
  variant<Variable*, Subroutine*> member;
  bool assignable()
  {
    return base->assignable() && member.is<Variable*>();
  }
  Variable* getRootVariable()
  {
    return base->getRootVariable();
  }
  bool hasSideEffects()
  {
    return base->hasSideEffects();
  }
  bool readsGlobals()
  {
    return base->readsGlobals();
  }
  bool isComputation()
  {
    return true;
  }
  void getReads(set<Variable*>& vars, bool lhs)
  {
    base->getReads(vars, lhs);
  }
  Variable* getWrite()
  {
    return base->getWrite();
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(base->hash());
    if(member.is<Variable*>())
      f.pump(member.get<Variable*>());
    else
      f.pump(member.get<Subroutine*>());
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct NewArray : public Expression
{
  NewArray(Type* elemType, vector<Expression*> dims);
  Type* elem;
  vector<Expression*> dims;
  void resolveImpl();
  bool assignable()
  {
    return false;
  }
  size_t hash() const
  {
    FNV1A f;
    for(size_t i = 0; i < dims.size(); i++)
      f.pump((i + 1) * dims[i]->hash());
    //can't hash the type
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct ArrayLength : public Expression
{
  ArrayLength(Expression* arr);
  Expression* array;
  void resolveImpl();
  bool assignable()
  {
    return false;
  }
  bool hasSideEffects()
  {
    return array->hasSideEffects();
  }
  bool readsGlobals()
  {
    return array->readsGlobals();
  }
  void getReads(set<Variable*>& vars, bool lhs)
  {
    array->getReads(vars, false);
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(5 * array->hash());
    return f.get();
  }
  bool isComputation()
  {
    return true;
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct IsExpr : public Expression
{
  IsExpr(Expression* b, Type* t)
  {
    base = b;
    ut = nullptr;
    optionIndex = -1;
    option = t;
    type = primitives[Prim::BOOL];
  }
  void resolveImpl();
  bool assignable()
  {
    return false;
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(optionIndex);
    f.pump(13 * base->hash());
    return f.get();
  }
  bool hasSideEffects()
  {
    return base->hasSideEffects();
  }
  void getReads(set<Variable*>& vars, bool lhs)
  {
    base->getReads(vars, lhs);
  }
  bool readsGlobals()
  {
    return base->readsGlobals();
  }
  bool isComputation()
  {
    return true;
  }
  bool operator==(const Expression& rhs) const;
  ostream& print(ostream& os);
  Expression* copy();
  Expression* base;
  UnionType* ut;
  int optionIndex;
  Type* option;
};

struct AsExpr : public Expression
{
  AsExpr(Expression* b, Type* t)
  {
    base = b;
    ut = nullptr;
    optionIndex = -1;
    type = t;
  }
  void resolveImpl();
  bool assignable()
  {
    return false;
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(optionIndex);
    f.pump(17 * base->hash());
    return f.get();
  }
  bool hasSideEffects()
  {
    return base->hasSideEffects();
  }
  bool readsGlobals()
  {
    return base->readsGlobals();
  }
  bool isComputation()
  {
    return true;
  }
  void getReads(set<Variable*>& vars, bool lhs)
  {
    base->getReads(vars, lhs);
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
  Expression* base;
  UnionType* ut;
  int optionIndex;
};

struct ThisExpr : public Expression
{
  ThisExpr(Scope* where);
  void resolveImpl();
  //structType is equal to type
  StructType* structType;
  bool assignable()
  {
    return true;
  }
  //note here: ThisExpr can read/write globals if "this"
  //is a global, but that can only be done through a proc all on
  //a global object. So it's safe to assume "this" is  
  size_t hash() const
  {
    //all ThisExprs are the same
    return 0xDEADBEEF;
  }
  bool operator==(const Expression& rhs) const
  {
    //in any context, "this" always refers to the same thing
    return dynamic_cast<const ThisExpr*>(&rhs);
  }
  Scope* usage;
  Expression* copy();
  ostream& print(ostream& os);
};

struct Converted : public Expression
{
  Converted(Expression* val, Type* dst);
  Expression* value;
  bool assignable()
  {
    return false;
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(23 * value->hash());
    return f.get();
  }
  bool hasSideEffects()
  {
    return value->hasSideEffects();
  }
  bool readsGlobals()
  {
    return value->readsGlobals();
  }
  void getReads(set<Variable*>& vars, bool lhs)
  {
    value->getReads(vars, false);
  }
  bool isComputation()
  {
    return true;
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct EnumExpr : public Expression
{
  EnumExpr(EnumConstant* ec);
  EnumConstant* value;
  bool assignable()
  {
    return false;
  }
  size_t hash() const
  {
    //EnumConstants are names (unique, pointer-identifiable)
    return fnv1a(value);
  }
  bool constant() const
  {
    return true;
  }
  bool operator<(const Expression& rhs) const
  {
    const EnumExpr& e = dynamic_cast<const EnumExpr&>(rhs);
    return value->value < e.value->value;
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct SimpleConstant : public Expression
{
  SimpleConstant(SimpleType* s);
  SimpleType* st;
  bool assignable()
  {
    return false;
  }
  bool constant() const
  {
    return true;
  }
  bool operator<(const Expression& rhs) const
  {
    return false;
  }
  size_t hash() const;
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

//DefaultValueExpr is just a placeholder
//When resolved, it's replaced by type->getDefaultValue()
struct DefaultValueExpr : public Expression
{
  DefaultValueExpr(Type* t_) : t(t_) {}
  void resolveImpl()
  {
    INTERNAL_ERROR;
  }
  bool assignable()
  {
    return false;
  }
  size_t hash() const
  {
    return 0;
  }
  Expression* copy()
  {
    return new DefaultValueExpr(t);
  }
  bool operator==(const Expression&) const
  {
    INTERNAL_ERROR;
    return false;
  }
  ostream& print(ostream& os)
  {
    INTERNAL_ERROR;
    return os;
  }
  Type* t;
};

struct UnresolvedExpr : public Expression
{
  UnresolvedExpr(string name, Scope* s);
  UnresolvedExpr(Member* name, Scope* s);
  UnresolvedExpr(Expression* base, Member* name, Scope* s);
  bool assignable()
  {
    return false;
  }
  void resolveImpl()
  {
    //Should never be called!
    //Should instead be replaced inside resolveExpr(...)
    INTERNAL_ERROR;
  }
  size_t hash() const
  {
    //UnresolvedExpr does not appear in a resolved AST
    return 0;
  }
  Expression* copy()
  {
    INTERNAL_ERROR;
    return nullptr;
  }
  ostream& print(ostream& os)
  {
    INTERNAL_ERROR;
    return os;
  }
  bool operator==(const Expression&) const
  {
    INTERNAL_ERROR;
    return false;
  }
  Expression* base; //null = no base
  Member* name;
  Scope* usage;
};

void resolveExpr(Expression*& expr);

#endif

