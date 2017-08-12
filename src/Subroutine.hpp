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
struct For;
struct While;

typedef variant<For*, While*> Loop;

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
  //subroutine whose body contains this block (passed down to child blocks that aren't 
  Subroutine* subr;
  //innermost loop whose body contains this block (or NULL if none)
  Loop* loop;
};

//Create any kind of Statement - adds to block
Statement* createStatement(Block* s, Parser::StatementNT* stmt);
//Given a VarDecl, add a new Variable to scope and then
//create an Assign statement if that variable is initialized
Statement* addLocalVariable(BlockScope* s, Parser::VarDecl* vd);
//Create a local variable with given name and type
Statement* addLocalVariable(BlockScope* s, string name, Type* type, Expression* init);

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
  //Even if the body is not a block, the loop has a BlockScope for containing the counter
  //Statement 0 is init(s), body, increment(s) in that order
  Block* loopBlock;
  Statement* init;
  Expression* condition;  //check this before each entry to loop body
  Statement* body;
  Statement* increment;
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
  Loop loop;
};

struct Continue : public Statement
{
  //this ctor checks that the statement is being used inside a loop
  Continue(BlockScope* s);
  Loop loop;
};

struct Print : public Statement
{
  Print(Parser::PrintNT* p, BlockScope* s);
  vector<Expression*> exprs;
};

struct Assertion : public Statement
{
  Assertion(Parser::Assertion* as, BlockScope* s);
  Expression* asserted;
};

struct Subroutine
{
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

