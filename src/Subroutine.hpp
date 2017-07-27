#ifndef SUBROUTINE_H
#define SUBROUTINE_H

#include "TypeSystem.hpp"
#include "Parser.hpp"
#include "Expression.hpp"
#include "Variable.hpp"
#include "Scope.hpp"

//Abstract middle-end format for functions, procedures and statements

struct Statement
{
};

struct Assign : public Statement
{
  Expression* lvalue;
  Expression* rvalue;
};

struct For : public Statement
{
  Type* counterType;
  Expression* start;
  Expression* condition;
  Statement* increment;
  Statement* body;
};

struct ForRange : public Statement
{
  Type* iterable;
  Statement* body;
};

struct While : public Statement
{
  Expression* condition;
  Statement* body;
};

struct If : public Statement
{
  Expression* condition;
  Statement* body;
};

struct IfElse : public Statement
{
  Expression* condition;
  Statement* trueBody;
  Statement* falseBody;
};

struct Return : public Statement
{
};

struct Break : public Statement
{
};

struct Continue : public Statement
{
};

struct Print : public Statement
{
  vector<Expression*> exprs;
};

struct Subroutine
{
  Scope* s;
  Type* retType;
  vector<Type*> argTypes;
  bool pure;              //true for functions and false for procedures
};

#endif

