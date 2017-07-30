#ifndef SUBROUTINE_H
#define SUBROUTINE_H

#include "TypeSystem.hpp"
#include "Parser.hpp"
#include "Expression.hpp"
#include "Scope.hpp"

//Abstract middle-end format for functions, procedures and statements

struct Statement
{};

//Block scope provides:
//  -scope for looking up variables/types
//  -BlockScope::Variable
Statement* createStatement(Parser::StatementNT* stmt, Parser::BlockScope* b);

struct Block : public Statement
{
  Block(Parser::Block* b, Scope* s);
  vector<Statement*> stmts;
};

struct NewVar : public Statement
{
  NewVar(Parser::VarDecl* vd, Scope* s);
};

struct Assign : public Statement
{
  Assign(Parser::VarAssign* 
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
  Statement* value; //can be null
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
  Subroutine(Scope* enclosing, Parser::Block* block) : s(enclosing) {}
  Scope* s;
  Type* retType;
  vector<Type*> argTypes;
  bool pure;              //true for functions and false for procedures
  vector<Statement*> statements;
  StructType* owner;      //the struct type with this subroutine as a non-static member (otherwise null)
};

struct Function : public Subroutine
{
  Function(Parser::FuncDef* a, Scope* enclosing);
};

struct Procedure : public Subroutine
{
  Procedure(Parser::ProcDef* a, Scope* enclosing);
};

#endif

