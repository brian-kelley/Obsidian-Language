#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "variant.h"

namespace MiddleEndExpr
{

struct Expression
{
  //Expression constructor will determine the type (implemented in subclasses)
  Expression(Scope* s);
  Scope* scope;
  //TODO: throw if any var decl with "auto" as type has an untyped RHS
  TypeSystem::Type* type;
  virtual bool assignable() = 0;
};

//Create a new Expression given one of the ExprN nonterminals
Expression* getExpression(Parser::Expr1* expr):
Expression* getExpression(Parser::Expr2* expr):
Expression* getExpression(Parser::Expr3* expr):
Expression* getExpression(Parser::Expr4* expr):
Expression* getExpression(Parser::Expr5* expr):
Expression* getExpression(Parser::Expr6* expr):
Expression* getExpression(Parser::Expr7* expr):
Expression* getExpression(Parser::Expr8* expr):
Expression* getExpression(Parser::Expr9* expr):
Expression* getExpression(Parser::Expr10* expr):
Expression* getExpression(Parser::Expr11* expr):
Expression* getExpression(Parser::Expr12* expr):

struct UnaryArith : public Expression
{
  //Precondition: ast->e is an Expr11::UnaryExpr
  UnaryArith(Scope* s, int op, Expression* expr);
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
  AP(Expression) lhs;
  AP(Expression) rhs;
  bool assignable()
  {
    return false;
  }
};

struct PrimitiveLiteral : public Expression
{
  PrimitiveLiteral(Scope* s, Parser::Expr12* ast);
  variant<None,
    IntLit*,
    FloatLit*,
    StrLit*,
    CharLit*,
    Parser::BoolLit*> value;
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
    return false;
  }
  vector<Expression*> members;
};

//Unlike CompoundLiteral, can get the type of TupleLiteral by itself
//as long as all members' types are known, so
//"auto tup = (1, 2, 3, 4.5, "hello");" is valid
struct TupleLiteral : public Expression
{
};

struct Indexed : public Expression
{
  Indexed(Scope* s, Parser::Expr12::ArrayIndex* ast);
  AP(Expression) array;
  AP(Expression) index;
  Parser::Expr12::ArrayIndex ast;
  bool assignable()
  {
    //can assign to any subscript of lvalue array or tuple
    return array->assignable();
  }
};

struct Call : public Expression
{
  Call(Scope* s, Parser::Call* ast);
  bool assignable()
  {
    return false;
  }
};

struct Var : public Expression
{
  Var(Scope* s, Parser::Member* ast);
  Variable* var;  //var must be looked up from current scope
  bool assignable()
  {
    //all variables are lvalues (no const)
    return true;
  }
};

}

#endif

