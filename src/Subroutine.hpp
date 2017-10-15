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
{
  virtual ~Statement() {}
};

//Statement types
struct Block;
struct Assign;
struct CallStmt;
struct For;
struct While;
struct If;
struct IfElse;
struct Return;
struct Break;
struct Continue;
struct Print;
struct Assertion;

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
  //Constructor for For loop body
  Block(Parser::For* lp, For* f, Block* parent);
  //Constructor for While loop body
  Block(Parser::While* lp, While* w, Block* parent);
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
Statement* addLocalVariable(BlockScope* s, string name, TypeSystem::Type* type, Expression* init);

struct Assign : public Statement
{
  Assign(Parser::VarAssign* va, BlockScope* s);
  Assign(Variable* target, Expression* e, Scope* s);
  Expression* lvalue;
  Expression* rvalue;
};

struct CallStmt : public Statement
{
  //Ctor for when it is known that Expr12 is a call
  CallStmt(Parser::Expr12* call, BlockScope* s);
  Procedure* called;
  Expression* base; //null for static call
  //a standalone procedure call just has arguments
  vector<Expression*> args;
};

struct For : public Statement
{
  //note: scope provided in Parser::For
  For(Parser::For* f, Block* b);
  Block* loopBlock;
  Statement* init;
  Expression* condition;  //check this before each entry to loop body
  Statement* increment;
};

struct While : public Statement
{
  While(Parser::While* w, Block* b);
  Block* loopBlock;
  Expression* condition;
};

struct If : public Statement
{
  If(Parser::If* i, Block* b);
  Expression* condition;
  Statement* body;
};

struct IfElse : public Statement
{
  IfElse(Parser::If* ie, Block* b);
  Expression* condition;
  Statement* trueBody;
  Statement* falseBody;
};

struct Return : public Statement
{
  Return(Parser::Return* r, Block* s);
  Expression* value; //can be null (void return)
  Subroutine* from;
};

struct Break : public Statement
{
  //this ctor checks that the statement is being used inside a loop
  Break(Block* s);
  Loop* loop;
};

struct Continue : public Statement
{
  //this ctor checks that the statement is being used inside a loop
  Continue(Block* s);
  Loop* loop;
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
  Subroutine(string name, Parser::TypeNT* ret, vector<Parser::Arg*>& args, Parser::Block* body);
  TypeSystem::Type* retType;
  vector<TypeSystem::Type*> argTypes;
  //Local variables in body's scope representing arguments, in order
  vector<Variable*> argVars;
  Block* body;
  virtual bool isPure() = 0;
  string name;
  bool isStatic;
  TypeSystem::StructType* owner;      //the struct type with this subroutine as a non-static member (otherwise null)
};

struct Function : public Subroutine
{
  Function(Parser::FuncDef* a);
  bool isPure();
};

struct Procedure : public Subroutine
{
  Procedure(Parser::ProcDef* a);
  bool isPure();
};

#endif

