#ifndef SUBROUTINE_H
#define SUBROUTINE_H

#include "TypeSystem.hpp"
#include "Parser.hpp"
#include "Expression.hpp"
#include "Scope.hpp"

/***************************************************************************/
// Subroutine: middle-end structures for program behavior and control flow //
/***************************************************************************/

struct Statement
{};

struct Subroutine;
struct Procedure; 

//Block: list of statements
struct Block : public Statement
{
  //Constructor for function/procedure body
  Block(Parser::Block* b, Subroutine* subr);
  //Constructor for block inside a function/procedure
  Block(Parser::Block* b, Block* parent);
  Parser::Block* ast;
  void addStatements();
  vector<Statement*> stmts;
  //scope of the block
  BlockScope* scope;
};

//Create any kind of Statement
Statement* createStatement(Block* b, Parser::StatementNT* stmt);
//Given a VarDecl, add a new Variable to scope and then
//create an Assign statement if that variable is initialized
void addLocalVariable(Block* b, Parser::VarDecl* vd);

struct Assign : public Statement
{
  Assign(Parser::VarAssign* va, BlockScope* s);
  Assign(Variable* target, Expression* e, BlockScope* s);
  Expression* lvalue;
  Expression* rvalue;
};

struct CallStmt : public Statement
{
  CallStmt(Parser::CallNT* c, BlockScope* s);
  Procedure* called;
  //a standalone procedure call just has arguments
  vector<Expression*> args;
};

struct For : public Statement
{
  For(Parser::For* f, BlockScope* s);
  //note: everything except body is auto-generated in case of ranged for
  //Even if the body is not a block, the loop introduces a BlockScope for the counter(s)
  BlockScope* scope;
  Expression* condition;
  Statement* increment;
  Statement* body;
};

struct While : public Statement
{
  While(Parser::While* w, BlockScope* s);
  Expression* condition;
  Statement* body;
};

struct If : public Statement
{
  If(Parser::If* i, BlockScope* s);
  Expression* condition;
  Statement* body;
};

struct IfElse : public Statement
{
  IfElse(Parser::IfElse* ie, BlockScope* s);
  Expression* condition;
  Statement* trueBody;
  Statement* falseBody;
};

struct Return : public Statement
{
  Return(Parser::Return* r, BlockScope* s);
  Statement* value; //can be null
};

struct Break : public Statement
{
  //this ctor checks that the statement is being used inside a loop
  Break(BlockScope* s);
};

struct Continue : public Statement
{
  //this ctor checks that the statement is being used inside a loop
  Continue(BlockScope* s);
};

struct Print : public Statement
{
  Print(Parser::PrintNT* p, Scope* s);
  vector<Expression*> exprs;
};

struct Assertion : public Statement
{
  Assertion(Parser::Assertion* as, Scope* s);
  Expression* asserted;
};

struct Subroutine
{
  Subroutine(BlockScope* enclosing, Parser::Block* block) : s(enclosing) {}
  //The scope of the subroutine (child of the enclosing scope)
  BlockScope* scope;
  Type* retType;
  vector<Type*> argTypes;
  vector<Statement*> statements;
  bool pure;              //true for functions and false for procedures
  string name;
  bool isStatic;
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

