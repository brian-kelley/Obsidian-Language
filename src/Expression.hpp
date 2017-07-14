#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "variant.h"

namespace Expression
{

enum struct UnaryOp
{
  BNOT,
  LNOT,
  MINUS
};

enum struct BinaryOp
{
  ADD,
  SUB,
  MUL,
  DIV,
  BOR,
  BAND,
  BXOR,
  LOR,
  LAND,
  LXOR
};

struct Expression
{
  //Expression constructor will determine the type (implemented in subclasses)
  Expression(Scope* s);
  Scope* scope;
  //note: can't get type directly from CompoundLiteral, leave it null
  //TODO: throw if any var decl with "auto" as type has an untyped RHS
  TypeSystem::Type* type;
  virtual bool assignable() = 0;
};

struct UnaryArith : public Expression
{
  UnaryArith(Scope* s, Oper* oper, Parser::Expr11* ast);
  UnaryOp op;
  AP(Expression) expr;
  bool assignable()
  {
    return false;
  }
};

struct BinaryArith : public Expression
{
  BinaryArith(Scope* s, Parser::Expr1* ast);
  BinaryArith(Scope* s, Parser::Expr2* ast);
  BinaryArith(Scope* s, Parser::Expr3* ast);
  BinaryArith(Scope* s, Parser::Expr4* ast);
  BinaryArith(Scope* s, Parser::Expr5* ast);
  BinaryArith(Scope* s, Parser::Expr6* ast);
  BinaryArith(Scope* s, Parser::Expr7* ast);
  BinaryArith(Scope* s, Parser::Expr8* ast);
  BinaryArith(Scope* s, Parser::Expr9* ast);
  BinaryArith(Scope* s, Parser::Expr10* ast);
  BinaryOp op;
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
    Parser::BoolLit*> lit;
  bool assignable()
  {
    return false;
  }
};

struct CompoundLiteral : public Expression
{
  CompoundLiteral(Scope* s, Parser::StructLit* ast);
  Parser::StructLit* ast;
  bool assignable()
  {
    return false;
  }
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

