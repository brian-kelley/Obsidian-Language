#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "Parser.hpp"
#include "TypeSystem.hpp"

namespace Expression
{

enum struct Operation
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
  //Expression constructor determines the type
  Expression(Parser::ExpressionNT* expr, Scope* s);
  bool lvalue();    //possible to assign to this?
  Scope* scope;
  Parser::ExpressionNT* e;
  TypeSystem::Type* type;
};

struct Arithmetic : public Expression
{
};

}

#endif

