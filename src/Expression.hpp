#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "AST.hpp"

struct Expression;
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
struct UnresolvedExpr;

//expr must be resolved
ostream& operator<<(ostream& os, Expression* expr);

struct Expression : public Node
{
  Expression() : type(nullptr) {}
  virtual void resolveImpl() {}
  virtual set<Variable*> getReads()
  {
    return set<Variable*>();
  }
  virtual set<Variable*> getWrites()
  {
    return set<Variable*>();
  }
  Type* type;
  //whether this works as an lvalue
  virtual bool assignable() = 0;
  //whether this is a compile-time constant
  virtual bool constant() const
  {
    return false;
  }
  //get the number of bytes required to store the constant
  //(is 0 for non-constants)
  virtual int getConstantSize()
  {
    return 0;
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
  //Is it worth doing CSE on this?
  virtual bool isComputation()
  {
    return false;
  }
  //Hash this expression (for use in unordered map/set)
  //Only hard requirement: if a == b, then hash(a) == hash(b)
  virtual size_t hash() const = 0;
  //implementation of operator<, but only supported by
  //constant expression types (minus MapConstant)
  virtual bool compareLess(const Expression& rhs) const
  {
    INTERNAL_ERROR;
  }
  //get a unique tag for this expression type
  //just used for comparing/ordering Expressions
  virtual int getTypeTag() const = 0;
  virtual Variable* getRootVariable() {INTERNAL_ERROR;}
  //deep copy (must already be resolved)
  virtual Expression* copy() = 0;
};

//compare expressions by value/semantics (not by pointer)
bool operator==(const Expression& lhs, const Expression& rhs);
inline bool operator!=(const Expression& lhs, const Expression& rhs)
{
  return !(lhs == rhs);
}

struct ExprEqual
{
  bool operator()(const Expression* lhs, const Expression* rhs) const
  {
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

//support general relational comparison for expressions, but this is only
//really implemented for constants (except MapConstant)
bool operator<(const Expression& lhs, const Expression& rhs);

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
  set<Variable*> getReads();
  int getTypeTag() const
  {
    return 0;
  }
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
  size_t hash() const
  {
    FNV1A f;
    f.pump(op);
    f.pump(expr->hash());
    return f.get();
  }
  Expression* copy();
};

bool operator==(const UnaryArith& lhs, const UnaryArith& rhs);

struct BinaryArith : public Expression
{
  BinaryArith(Expression* lhs, int op, Expression* rhs);
  int op;
  Expression* lhs;
  Expression* rhs;
  void resolveImpl();
  set<Variable*> getReads();
  bool assignable()
  {
    return false;
  }
  int getTypeTag() const
  {
    return 1;
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
  bool commutative()
  {
    return operCommutativeTable[op];
  }
  size_t hash() const
  {
    FNV1A f;
    size_t lhsHash = lhs->hash();
    size_t rhsHash = rhs->hash();
    //Make sure that "a op b" and "b op a" hash the same if op is commutative
    if(operCommutativeTable[op] && lhsHash > rhsHash)
      std::swap(lhsHash, rhsHash);
    f.pump(op);
    f.pump(lhsHash);
    f.pump(rhsHash);
    return f.get();
  }
  Expression* copy();
};

bool operator==(const BinaryArith& lhs, const BinaryArith& rhs);

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
  bool compareLess(const Expression& rhs) const
  {
    const IntConstant& rhsInt = dynamic_cast<const IntConstant&>(rhs);
    if(isSigned())
      return sval < rhsInt.sval;
    else
      return uval < rhsInt.uval;
  }
  int getTypeTag() const
  {
    return 2;
  }
  Expression* copy();
};

bool operator==(const IntConstant& lhs, const IntConstant& rhs);

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
  bool compareLess(const Expression& rhs) const
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
  int getTypeTag() const
  {
    return 3;
  }
  Expression* copy();
};

bool operator==(const FloatConstant& lhs, const FloatConstant& rhs);

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
  bool compareLess(const Expression& rhs) const
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
  int getTypeTag() const
  {
    return 4;
  }
  Expression* copy();
};

bool operator==(const StringConstant& lhs, const StringConstant& rhs);

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
  bool compareLess(const Expression& rhs) const
  {
    const CharConstant& rhsChar = dynamic_cast<const CharConstant&>(rhs);
    return value < rhsChar.value;
  }
  size_t hash() const
  {
    return fnv1a(value);
  }
  int getTypeTag() const
  {
    return 5;
  }
  Expression* copy();
};

bool operator==(const CharConstant& lhs, const CharConstant& rhs);

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
  bool compareLess(const Expression& rhs) const
  {
    const BoolConstant& b = dynamic_cast<const BoolConstant&>(rhs);
    return !value && b.value;
  }
  size_t hash() const
  {
    //don't use FNV-1a here since it's way more
    //expensive than just doing exact comparison
    if(value)
      return 0x12345678;
    else
      return 0x98765432;
  }
  int getTypeTag() const
  {
    return 6;
  }
  Expression* copy();
};

bool operator==(const CharConstant& lhs, const CharConstant& rhs);

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
    //the order of key-value pairs in values is deterministic,
    //so use that to hash
    FNV1A f;
    f.pump(getTypeTag());
    for(auto& kv : values)
    {
      f.pump(kv.first->hash());
      f.pump(kv.second->hash());
    }
    return f.get();
  }
  int getTypeTag() const
  {
    return 7;
  }
  Expression* copy();
};

bool operator==(const MapConstant& lhs, const MapConstant& rhs);

//UnionConstant only used in IR/optimization
//expr->type exactly matches exactly one of ut's options
//(which is guaranteed by semantic checking/implicit conversions)
struct UnionConstant : public Expression
{
  UnionConstant(Expression* expr, Type* t, UnionType* ut);
  bool assignable()
  {
    return false;
  }
  bool constant() const
  {
    return true;
  }
  int getTypeTag() const
  {
    return 8;
  }
  bool compareLess(const Expression& rhs) const
  {
    const UnionConstant& u = dynamic_cast<const UnionConstant&>(rhs);
    if(option < u.option)
      return true;
    else if(option > u.option)
      return false;
    return value->compareLess(*u.value);
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(getTypeTag());
    f.pump(option);
    f.pump(value->hash());
    return f.get();
  }
  Expression* copy();
  UnionType* unionType;
  Expression* value;
  int option;
};

bool operator==(const UnionConstant& lhs, const UnionConstant& rhs);

//it is impossible to determine the type of a CompoundLiteral by itself
//(CompoundLiteral covers array, struct and tuple literals)
struct CompoundLiteral : public Expression
{
  CompoundLiteral(vector<Expression*>& mems);
  void resolveImpl();
  bool assignable()
  {
    return lvalue;
  }
  vector<Expression*> members;
  //(set during resolution): is every member an lvalue?
  bool lvalue;
  set<Variable*> getReads();
  set<Variable*> getWrites();
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
  int getTypeTag() const
  {
    return 9;
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
  bool readsGlobals()
  {
    for(auto m : members)
    {
      if(m->readsGlobals())
        return true;
    }
    return false;
  }
  bool compareLess(const Expression& rhs) const
  {
    const CompoundLiteral& cl = dynamic_cast<const CompoundLiteral&>(rhs);
    //lex compare
    size_t n = std::min(members.size(), cl.members.size());
    for(size_t i = 0; i < n; i++)
    {
      if(members[i]->compareLess(*cl.members[i]))
        return true;
      else if(*members[i] != *cl.members[i])
        return false;
    }
    return n == members.size();
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(getTypeTag());
    for(auto m : members)
      f.pump(m->hash());
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
  Expression* copy();
};

bool operator==(const CompoundLiteral& lhs, const CompoundLiteral& rhs);

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
  int getTypeTag() const
  {
    return 10;
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
  size_t hash() const
  {
    FNV1A f;
    f.pump(getTypeTag());
    f.pump(group->hash());
    f.pump(index->hash());
    return f.get();
  }
  Expression* copy();
  set<Variable*> getReads();
  set<Variable*> getWrites();
};

bool operator==(const Indexed& lhs, const Indexed& rhs);

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
  int getTypeTag() const
  {
    return 11;
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
  size_t hash() const
  {
    FNV1A f;
    f.pump(getTypeTag());
    f.pump(callable->hash());
    for(auto a : args)
      f.pump(a->hash());
    return f.get();
  }
  Expression* copy();
  //TODO: do evaluate calls in optimizing mode
  set<Variable*> getReads();
};

bool operator==(const CallExpr& lhs, const CallExpr& rhs);

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
  int getTypeTag() const
  {
    return 12;
  }
  Variable* getRootVariable()
  {
    return var;
  }
  bool hasSideEffects()
  {
    return false;
  }
  bool readsGlobals();
  size_t hash() const
  {
    //variables are uniquely identifiable by pointer
    return fnv1a(var);
  }
  Expression* copy();
  set<Variable*> getReads();
  set<Variable*> getWrites();
};

bool operator==(const VarExpr& lhs, const VarExpr& rhs);

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
  int getTypeTag() const
  {
    return 13;
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
  size_t hash() const
  {
    FNV1A f;
    f.pump(thisObject);
    f.pump(subr);
    f.pump(exSubr);
    return f.get();
  }
  Expression* copy();
  Subroutine* subr;
  ExternalSubroutine* exSubr;
  Expression* thisObject; //null for static/extern
};

bool operator==(const SubroutineExpr& lhs, const SubroutineExpr& rhs);

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
  int getTypeTag() const
  {
    return 15;
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
  Expression* copy();
  set<Variable*> getReads();
  set<Variable*> getWrites();
};

bool operator==(const StructMem& lhs, const StructMem& rhs);

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
    f.pump(getTypeTag());
    for(auto d : dims)
      f.pump(d->hash());
    //can't hash the type
    return f.get();
  }
  int getTypeTag() const
  {
    return 16;
  }
  Expression* copy();
};

bool operator==(const NewArray& lhs, const NewArray& rhs);

struct ArrayLength : public Expression
{
  ArrayLength(Expression* arr);
  Expression* array;
  void resolveImpl();
  bool assignable()
  {
    return false;
  }
  int getTypeTag() const
  {
    return 17;
  }
  bool hasSideEffects()
  {
    return array->hasSideEffects();
  }
  bool readsGlobals()
  {
    return array->readsGlobals();
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(getTypeTag());
    f.pump(array->hash());
    return f.get();
  }
  bool isComputation()
  {
    return true;
  }
  Expression* copy();
  set<Variable*> getReads();
};

bool operator==(const ArrayLength& lhs, const ArrayLength& rhs);

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
  set<Variable*> getReads()
  {
    return base->getReads();
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(getTypeTag());
    f.pump(optionIndex);
    f.pump(base->hash());
    return f.get();
  }
  int getTypeTag() const
  {
    return 18;
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
  Expression* copy();
  Expression* base;
  UnionType* ut;
  int optionIndex;
  Type* option;
};

bool operator==(const IsExpr& lhs, const IsExpr& rhs);

struct AsExpr : public Expression
{
  AsExpr(Expression* b, Type* t)
  {
    base = b;
    ut = nullptr;
    optionIndex = -1;
    option = t;
    type = t;
  }
  void resolveImpl();
  bool assignable()
  {
    return false;
  }
  set<Variable*> getReads()
  {
    return base->getReads();
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(getTypeTag());
    f.pump(optionIndex);
    f.pump(base->hash());
    return f.get();
  }
  int getTypeTag() const
  {
    return 19;
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
  Expression* copy();
  Expression* base;
  UnionType* ut;
  int optionIndex;
  Type* option;
};

bool operator==(const AsExpr& lhs, const AsExpr& rhs);

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
  int getTypeTag() const
  {
    return 20;
  }
  //note here: ThisExpr can read/write globals if "this"
  //is a global, but that can only be done through a proc all on
  //a global object. So it's safe to assume "this" is  
  size_t hash() const
  {
    //all ThisExprs are the same
    return 0xDEADBEEF;
  }
  Scope* usage;
  Expression* copy();
};

struct Converted : public Expression
{
  Converted(Expression* val, Type* dst);
  Expression* value;
  bool assignable()
  {
    return value->assignable();
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(getTypeTag());
    f.pump(value->hash());
    return f.get();
  }
  int getTypeTag() const
  {
    return 21;
  }
  bool hasSideEffects()
  {
    return value->hasSideEffects();
  }
  bool readsGlobals()
  {
    return value->readsGlobals();
  }
  bool isComputation()
  {
    return true;
  }
  Expression* copy();
  set<Variable*> getReads();
};

bool operator==(const Converted& lhs, const Converted& rhs);

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
  bool compareLess(const Expression& rhs) const
  {
    const EnumExpr& e = dynamic_cast<const EnumExpr&>(rhs);
    if(value->et->underlying->isSigned)
      return value->sval < e.value->sval;
    return value->uval < e.value->uval;
  }
  int getTypeTag() const
  {
    return 22;
  }
  Expression* copy();
};

bool operator==(const EnumExpr& lhs, const EnumExpr& rhs);

struct SimpleConstant : public Expression
{
  SimpleConstant(SimpleType* s);
  SimpleType* st;
  size_t hash() const
  {
    return fnv1a(st);
  }
  bool assignable()
  {
    return false;
  }
  bool constant() const
  {
    return true;
  }
  bool compareLess(const Expression& rhs) const
  {
    return false;
  }
  int getTypeTag() const
  {
    return 23;
  }
  Expression* copy();
};

bool operator==(const SimpleConstant& lhs, const SimpleConstant& rhs);

struct UnresolvedExpr : public Expression
{
  UnresolvedExpr(string name, Scope* s);
  UnresolvedExpr(Member* name, Scope* s);
  UnresolvedExpr(Expression* base, Member* name, Scope* s);
  Expression* base; //null = no base
  Member* name;
  Scope* usage;
  bool assignable()
  {
    return false;
  }
  void resolveImpl()
  {
    INTERNAL_ERROR;
  }
  int getTypeTag() const
  {
    return 14;
  }
  size_t hash() const
  {
    //UnresolvedExpr does not appear in a resolved AST
    INTERNAL_ERROR;
    return 0;
  }
  Expression* copy()
  {
    INTERNAL_ERROR;
    return nullptr;
  }
};

void resolveExpr(Expression*& expr);

#endif

