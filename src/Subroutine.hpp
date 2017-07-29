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
  Scope* s;
};

struct Assign : public Statement
{
  Expression* lvalue;
  Expression* rvalue;
};

struct Block : public Statement
{
  vector<Statement*> stmts;
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
  Statement* value;
};

struct Break : public Statement
{};

struct Continue : public Statement
{};

struct Print : public Statement
{
  vector<Expression*> exprs;
};

struct Subroutine
{
  Subroutine(Scope* enclosing) : s(enclosing) {}
  Scope* s;
  Type* retType;
  vector<Type*> argTypes;
  bool pure;              //true for functions and false for procedures
  vector<Statement*> statements;
  StructType* owner;      //the struct type with this as member (or null if non-member)
};

struct Function : public Subroutine
{
  Function(Parser::FuncDef* a, Scope* enclosing);
  Parser::FuncDef* ast;
};

struct Procedure : public Subroutine
{
  Procedure(Parser::ProcDef* a, Scope* enclosing);
  Parser::ProcDef* ast;
};

#endif

