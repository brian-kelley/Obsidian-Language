#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "AST.hpp"

struct Expression : public Node
{
  Expression() : type(nullptr) {}
  virtual void resolve(bool final) {}
  TypeSystem::Type* type;
  //whether this works as an lvalue
  virtual bool assignable() = 0;
};

//Resolve an expression in-place
//this is needed because resolved expr may have
//different type than unresolved
Expression* resolveExpr(Expression*& expr, bool final);

//Subclasses of Expression
struct UnaryArith;
struct BinaryArith;
struct IntLiteral;
struct FloatLiteral;
struct StringLiteral;
struct CharLiteral;
struct BoolLiteral;
struct CompoundLiteral;
struct Indexed;
struct CallExpr;
struct VarExpr;
struct NewArray;
struct Converted;
struct ArrayLength;
struct ThisExpr;
struct ErrorVal;
struct UnresolvedExpr;

//process one name as part of Expr12 or Expr12RHS
//returns true iff root is a valid expression that uses all names
void processExpr12Name(string name, bool& isFinal, bool first, Expression*& root, Scope*& scope);

//Assuming expr is a struct type, get the struct scope
//Otherwise, display relevant errors
StructScope* scopeForExpr(Expression* expr);

struct UnaryArith : public Expression
{
  UnaryArith(int op, Expression* expr);
  int op;
  Expression* expr;
  bool assignable()
  {
    return false;
  }
  void resolve(bool final);
};

struct BinaryArith : public Expression
{
  BinaryArith(Expression* lhs, int op, Expression* rhs);
  int op;
  Expression* lhs;
  Expression* rhs;
  bool assignable()
  {
    return false;
  }
  void resolve(bool final);
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
};

struct StringLiteral : public Expression
{
  StringLiteral(StrLit* ast);
  string value;
  bool assignable()
  {
    return false;
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
};

struct BoolLiteral : public Expression
{
  BoolLiteral(Parser::BoolLit* ast);
  bool value;
  bool assignable()
  {
    return false;
  }
};

//it is impossible to determine the type of a CompoundLiteral by itself
//CompoundLiteral covers both array and struct literals
struct CompoundLiteral : public Expression
{
  CompoundLiteral(vector<Expression*> mems);
  void resolve(bool final);
  bool assignable()
  {
    return lvalue;
  }
  vector<Expression*> members;
  bool lvalue;
};

struct Indexed : public Expression
{
  Indexed(Expression* grp, Expression* ind);
  void resolve(bool final);
  Expression* group; //the array or tuple being subscripted
  Expression* index;
  bool assignable()
  {
    return group->assignable();
  }
};

struct CallExpr : public Expression
{
  CallExpr(Expression* callable, vector<Expression*>& args);
  void resolve(bool final);
  Expression* callable;
  vector<Expression*> args;
  bool assignable()
  {
    return false;
  }
};

struct VarExpr : public Expression
{
  VarExpr(Variable* v, Scope* s);
  VarExpr(Variable* v);
  void resolve(bool final);
  Variable* var;  //var must be looked up from current scope
  Scope* scope;
  bool assignable()
  {
    //all variables are lvalues
    return true;
  }
};

//Expression to represent constant callable
//May be standalone, or may be applied to an object
struct SubroutineExpr : public Expression
{
  SubroutineExpr(Subroutine* s);
  SubroutineExpr(Expression* thisObj, Subroutine* s);
  SubroutineExpr(ExternalSubroutine* es);
  void resolve(bool final);
  bool assignable()
  {
    return false;
  }
  Subroutine* subr;
  ExternalSubroutine* exSubr;
  Expression* thisObject; //null for static/extern
};

struct UnresolvedExpr : public Expression
{
  UnresolvedExpr(string name, Scope* s);
  UnresolvedExpr(Parser::Member* name, Scope* s);
  UnresolvedExpr(Expression* base, Parser::Member* name, Scope* s);
  Expression* base; //null = no base
  Parser::Member* name;
  Scope* usage;
  bool assignable()
  {
    return false;
  }
};

struct StructMem : public Expression
{
  StructMem(Expression* base, Variable* var);
  void resolve(bool final);
  Expression* base;           //base->type is always StructType
  Variable* member;           //member must be a member of base->type
  bool assignable()
  {
    return base->assignable() && member.is<Variable*>();
  }
};

struct NewArray : public Expression
{
  NewArray(UnresolvedType* elemType, vector<Expression*> dims);
  UnresolvedType* elem;
  vector<Expression*> dims;
  void resolve(bool final);
  bool assignable()
  {
    return false;
  }
};

struct ArrayLength : public Expression
{
  ArrayLength(Expression* arr);
  Expression* array;
  void resolve(bool final);
  bool assignable()
  {
    return false;
  }
};

struct ThisExpr : public Expression
{
  ThisExpr(Scope* where);
  //structType == (StructType*) type,
  //structType is only for convenience
  TypeSystem::StructType* structType;
  bool assignable()
  {
    return true;
  }
};

struct Converted : public Expression
{
  Converted(Expression* val, TypeSystem::Type* dst);
  Expression* value;
  bool assignable()
  {
    return value->assignable();
  }
};

struct EnumExpr : public Expression
{
  EnumExpr(TypeSystem::EnumConstant* ec);
  int64_t value;
  bool assignable()
  {
    return false;
  }
};

struct ErrorVal : public Expression
{
  ErrorVal();
  bool assignable()
  {
    return false;
  }
};

void resolveExpr(Expression*& expr, bool final);

#endif

