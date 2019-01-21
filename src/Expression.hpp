#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "AST.hpp"

struct Expression;

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
  virtual bool constant()
  {
    return false;
  }
  //get the number of bytes required to store the constant
  //(is 0 for non-constants)
  virtual int getConstantSize()
  {
    return 0;
  }
  //Can evaluating this expr have side effects?
  virtual bool hasSideEffects()
  {
    return false;
  }
  //Is it worth doing CSE on this?
  virtual bool isComputation()
  {
    return false;
  }
  //get a unique tag for this expression type
  //just used for comparing/ordering Expressions
  virtual int getTypeTag() const = 0;
  virtual Variable* getRootVariable() {INTERNAL_ERROR;}
  //deep copy (must already be resolved)
  virtual Expression* copy() = 0;
};

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

//Assuming expr is a struct type, get the struct scope
//Otherwise, display relevant errors
Scope* scopeForExpr(Expression* expr);

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
  bool isComputation()
  {
    return true;
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
  bool isComputation()
  {
    return true;
  }
  bool commutative()
  {
    return operCommutativeTable[op];
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
  bool constant()
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
    return primitives[Prim::DOUBLE] == this->type;
  }
  float fp;
  double dp;
  FloatConstant* binOp(int op, FloatConstant* rhs);
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
  int getConstantSize()
  {
    return ((FloatType*) type)->size;
  }
  Expression* convert(Type* t);
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
  bool constant()
  {
    return true;
  }
  int getConstantSize()
  {
    return 16 + value.length() + 1;
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
  bool constant()
  {
    return true;
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
  bool constant()
  {
    return true;
  }
  int getConstantSize()
  {
    return 1;
  }
  int getTypeTag() const
  {
    return 6;
  }
  Expression* copy();
};

bool operator==(const CharConstant& lhs, const CharConstant& rhs);

struct ExprCompare
{
  bool operator()(const Expression* lhs, const Expression* rhs) const;
};

//Map constant: hold set of constant key-value pairs
//Relies on operator== and operator< for Expressions
struct MapConstant : public Expression
{
  MapConstant(MapType* mt);
  map<Expression*, Expression*, ExprCompare> values;
  bool constant()
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
  bool constant()
  {
    return true;
  }
  int getTypeTag() const
  {
    return 8;
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
  bool constant()
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
  bool isComputation()
  {
    return true;
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
  bool hasSideEffects()
  {
    //All procs are assumed to have side effects
    if(((CallableType*) callable->type)->isProc())
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
  bool isComputation()
  {
    return true;
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
    return var->isGlobal();
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
  bool constant()
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
  Expression* copy();
  Subroutine* subr;
  ExternalSubroutine* exSubr;
  Expression* thisObject; //null for static/extern
};

bool operator==(const SubroutineExpr& lhs, const SubroutineExpr& rhs);

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
  Expression* copy()
  {
    INTERNAL_ERROR;
    return nullptr;
  }
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
  int getTypeTag() const
  {
    return 18;
  }
  bool hasSideEffects()
  {
    return base->hasSideEffects();
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
  int getTypeTag() const
  {
    return 19;
  }
  bool hasSideEffects()
  {
    return base->hasSideEffects();
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
  int getTypeTag() const
  {
    return 21;
  }
  bool hasSideEffects()
  {
    return value->hasSideEffects();
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
  bool constant()
  {
    return true;
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
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
  int getTypeTag() const
  {
    return 23;
  }
  Expression* copy();
};

bool operator==(const SimpleConstant& lhs, const SimpleConstant& rhs);

void resolveExpr(Expression*& expr);

//compare expressions by value/semantics (not by pointer)
bool operator==(const Expression& lhs, const Expression& rhs);
inline bool operator!=(const Expression& lhs, const Expression& rhs)
{
  return !(lhs == rhs);
}

#endif

