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
  bool lvalue();    //possible to assign to this?
  Scope* scope;
  TypeSystem::Type* type;
};

struct UnaryArith : public Expression
{
  UnaryArith(Scope* s, Oper* oper, Parser::Expr11* ast);
  UnaryOp op;
  AP(Expression) expr;
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
};

struct CompoundLiteral : public Expression
{
  CompoundLiteral(Scope* s, Parser::StructLit* ast);
  Parser::StructLit* ast;
};

}

#endif

