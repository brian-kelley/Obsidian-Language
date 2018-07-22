#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "AST.hpp"

struct Expression : public Node
{
  Expression() : type(nullptr) {}
  virtual void resolveImpl(bool final) {}
  //Find set of read (input) or write (output) variables
  virtual set<Variable*> getReads()
  {
    return set<Variable*>();
  }
  //getWrites assume this is the LHS
  //so it's not implemented for RHS-only exprs
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
};

//Subclasses of Expression
//Constants/literals
struct IntLiteral;
struct FloatLiteral;
struct StringLiteral;
struct CharLiteral;
struct BoolLiteral;
struct TypedIntConstant;
struct TypedFloatConstant;
struct MapConstant;
struct CompoundLiteral;
//Arithmetic
struct UnaryArith;
struct BinaryArith;
struct Indexed;
struct CallExpr;
struct VarExpr;
struct NewArray;
struct Converted;
struct ArrayLength;
struct ThisExpr;
struct ErrorVal;
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
  void resolveImpl(bool final);
  set<Variable*> getReads();
  bool constant()
  {
    return expr->constant();
  }
};

struct BinaryArith : public Expression
{
  BinaryArith(Expression* lhs, int op, Expression* rhs);
  int op;
  Expression* lhs;
  Expression* rhs;
  void resolveImpl(bool final);
  set<Variable*> getReads();
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return lhs->constant() && rhs->constant();
  }
};

struct IntLiteral : public Expression
{
  IntLiteral(IntLit* ast);
  IntLiteral(uint64_t val);
  uint64_t value;
  bool assignable()
  {
    return false;
  }
  private:
  void setType(); //called by both constructors
  bool constant()
  {
    return true;
  }
};

struct FloatLiteral : public Expression
{
  FloatLiteral(FloatLit* ast);
  FloatLiteral(double val);
  double value;
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
};

struct StringLiteral : public Expression
{
  StringLiteral(StrLit* ast);
  string value;
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
};

struct CharLiteral : public Expression
{
  CharLiteral(CharLit* ast);
  char value;
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
};

struct BoolLiteral : public Expression
{
  BoolLiteral(bool v);
  bool value;
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
};

struct TypedIntConstant : public Expression
{
#define INT_CTOR(prim, ctype) \
  TypedIntConstant(ctype val) \
  { \
    type = primitives[Prim::prim]; \
    intType = (IntegerType*) type; \
  }
  INT_CTOR(BYTE, int8_t)
  INT_CTOR(UBYTE, uint8_t)
  INT_CTOR(SHORT, int16_t)
  INT_CTOR(USHORT, uint16_t)
  INT_CTOR(INT, int32_t)
  INT_CTOR(UINT, uint32_t)
  INT_CTOR(LONG, int64_t)
  INT_CTOR(ULONG, uint64_t)
#undef INT_CTOR
  //only one of these actually holds the value
  uint64_t uval;
  int64_t sval;
  IntegerType* intType;
};

struct TypedFloatConstant : public Expression
{
  TypedFloatConstant(float val)
  {
    type = primitives[Prim::FLOAT];
    dp = false;
    fval = val;
  }
  TypedFloatConstant(double val)
  {
    type = primitives[Prim::DOUBLE];
    dp = false;
    dval = val;
  }
  bool dp; //false = float, true = double
  float fval;
  double dval;
};

struct MapConstant : public Expre
{
};

//it is impossible to determine the type of a CompoundLiteral by itself
//CompoundLiteral covers both array and struct literals
struct CompoundLiteral : public Expression
{
  CompoundLiteral(vector<Expression*>& mems);
  void resolveImpl(bool final);
  bool assignable()
  {
    return lvalue;
  }
  vector<Expression*> members;
  bool lvalue;
  set<Variable*> getReads();
  set<Variable*> getWrites();
  bool constant()
  {
    bool c = true;
    for(auto m : members)
    {
      if(!m->constant())
      {
        c = false;
        break;
      }
    }
    return c;
  }
};

struct Indexed : public Expression
{
  Indexed(Expression* grp, Expression* ind);
  void resolveImpl(bool final);
  Expression* group; //the array or tuple being subscripted
  Expression* index;
  bool assignable()
  {
    return group->assignable();
  }
  bool constant()
  {
    return group->constant() && index->constant();
  }
  set<Variable*> getReads();
  set<Variable*> getWrites();
};

struct CallExpr : public Expression
{
  CallExpr(Expression* callable, vector<Expression*>& args);
  void resolveImpl(bool final);
  Expression* callable;
  vector<Expression*> args;
  bool assignable()
  {
    return false;
  }
  //TODO: do evaluate calls in optimizing mode
  set<Variable*> getReads();
};

struct VarExpr : public Expression
{
  VarExpr(Variable* v, Scope* s);
  VarExpr(Variable* v);
  void resolveImpl(bool final);
  Variable* var;  //var must be looked up from current scope
  Scope* scope;
  bool assignable()
  {
    //all variables are lvalues
    return true;
  }
  set<Variable*> getReads();
  set<Variable*> getWrites();
};

//Expression to represent constant callable
//May be standalone, or may be applied to an object
struct SubroutineExpr : public Expression
{
  SubroutineExpr(Subroutine* s);
  SubroutineExpr(Expression* thisObj, Subroutine* s);
  SubroutineExpr(ExternalSubroutine* es);
  void resolveImpl(bool final);
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
  Subroutine* subr;
  ExternalSubroutine* exSubr;
  Expression* thisObject; //null for static/extern
};

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
};

struct StructMem : public Expression
{
  StructMem(Expression* base, Variable* var);
  StructMem(Expression* base, Subroutine* subr);
  void resolveImpl(bool final);
  Expression* base;           //base->type is always StructType
  variant<Variable*, Subroutine*> member;
  bool assignable()
  {
    return base->assignable() && member.is<Variable*>();
  }
  bool constant()
  {
    return base->constant();
  }
  set<Variable*> getReads();
  set<Variable*> getWrites();
};

struct NewArray : public Expression
{
  NewArray(Type* elemType, vector<Expression*> dims);
  Type* elem;
  vector<Expression*> dims;
  void resolveImpl(bool final);
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    for(auto d : dims)
    {
      if(!d->constant())
        return false;
    }
    return true;
  }
};

struct ArrayLength : public Expression
{
  ArrayLength(Expression* arr);
  Expression* array;
  void resolveImpl(bool final);
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return array->constant();
  }
  set<Variable*> getReads();
};

struct ThisExpr : public Expression
{
  ThisExpr(Scope* where);
  //structType is equal to type
  StructType* structType;
  bool assignable()
  {
    return true;
  }
};

struct Converted : public Expression
{
  Converted(Expression* val, Type* dst);
  Expression* value;
  bool assignable()
  {
    return value->assignable();
  }
  bool constant()
  {
    return value->constant();
  }
  set<Variable*> getReads();
};

struct EnumExpr : public Expression
{
  EnumExpr(EnumConstant* ec);
  int64_t value;
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
};

struct ErrorVal : public Expression
{
  ErrorVal();
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
};

void resolveExpr(Expression*& expr, bool final);

//expr must be resolved
ostream& operator<<(ostream& os, Expression* expr);

#endif

