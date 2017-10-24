#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "variant.h"

struct Expression
{
  //Expression constructor will determine the type (implemented in subclasses)
  Expression(Scope* s);
  Scope* scope;
  TypeSystem::Type* type;
  virtual bool assignable() = 0;
};

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

//Create a new Expression given one of the ExprN nonterminals
template<typename NT>
Expression* getExpression(Scope* s, NT* expr);

//apply a single Expr12RHS to the right of an expression
Expression* applyExpr12RHS(Scope* s, Expression* root, Parser::Expr12RHS* e12);

struct UnaryArith : public Expression
{
  //Precondition: ast->e is an Expr11::UnaryExpr
  UnaryArith(int op, Expression* expr);
  int op;
  Expression* expr;
  bool assignable()
  {
    return false;
  }
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
  CharLit* ast;
  char value;
  bool assignable()
  {
    return false;
  }
};

struct BoolLiteral : public Expression
{
  BoolLiteral(Parser::BoolLit* ast);
  Parser::BoolLit* ast;
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
  CompoundLiteral(Scope* s, Parser::StructLit* ast);
  Parser::StructLit* ast;
  bool assignable()
  {
    return lvalue;
  }
  vector<Expression*> members;
  bool lvalue;
};

struct Indexed : public Expression
{
  //Indexed(Scope* s, Parser::Expr12::ArrayIndex* ast);
  Indexed(Scope* s, Expression* grp, Expression* ind);
  Expression* group; //the array or tuple being subscripted
  Expression* index;
  bool assignable()
  {
    return group->assignable();
  }
  private:
  void semanticCheck(); //called by both constructors
};

struct CallExpr : public Expression
{
  CallExpr(Scope* s, Subroutine* subr, vector<Expression*>& args);
  Subroutine* subr;
  Expression* base;
  vector<Expression*> args;
  bool assignable()
  {
    return false;
  }
};

struct MethodExpr : public Expression
{
  MethodExpr(Scope* s, Expression* thisObject, Subroutine* subr, vector<Expression*>& args);
  Expression* thisObject;
  Subroutine* subr;
};

struct VarExpr : public Expression
{
  VarExpr(Scope* s, Parser::Member* ast);
  VarExpr(Scope* s, Variable* v);
  Variable* var;  //var must be looked up from current scope
  bool assignable()
  {
    //all variables are lvalues
    return true;
  }
};

//Expression to represent constant callable
struct SubroutineExpr : public Expression
{
  SubroutineExpr(Scope* scope, Subroutine* s);
  Subroutine* subr;
};

struct StructMem : public Expression
{
  StructMem(Scope* s, Expression* base, string member);
  Expression* base;           //base->type is always a StructType
  vector<int> memberIndices;  //index of the member in base->type->members
  bool assignable()
  {
    return base->assignable();
  }
};

struct NewArray : public Expression
{
  NewArray(Scope* s, Parser::NewArrayNT* ast);
  vector<Expression*> dims;
  bool assignable()
  {
    return false;
  }
};

struct ArrayLength : public Expression
{
  ArrayLength(Expression* arr);
  Expression* array;
};

//Temporary variable (only used in backend)
//id should always come from C::getIdentifier()
struct TempVar : public Expression
{
  TempVar(string id, TypeSystem::Type* t, Scope* s);
  string ident;
  bool assignable()
  {
    return true;
  }
};

#endif

