#ifndef SUBROUTINE_H
#define SUBROUTINE_H

#include "TypeSystem.hpp"
#include "Parser.hpp"
#include "Expression.hpp"
#include "Scope.hpp"
#include "DeferredLookup.hpp"

/***************************************************************************/
// Subroutine: middle-end structures for program behavior and control flow //
/***************************************************************************/

struct Statement
{
  //Check that the statement doesn't violate the purity
  //requirement of functions (s should be innermost fn scope)
  virtual void checkPurity(Scope* s) {}
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
struct Switch;
struct Match;

struct Subroutine;
struct Procedure; 
struct For;
struct While;

struct Test;

//Loop (used by continue)
typedef variant<None, For*, While*> Loop;
//Breakable (used by break)
typedef variant<None, For*, While*, Switch*> Breakable;

//Block: list of statements
struct Block : public Statement
{
  //Constructor for function/procedure body
  Block(Parser::Block* b, BlockScope* s, Subroutine* subr);
  //Constructor for empty block (used inside For::For)
  Block(BlockScope* s, Block* parent);
  //Constructor for block inside a function/procedure
  Block(Parser::Block* b, BlockScope* s, Block* parent);
  //Constructor for For loop body
  Block(Parser::For* forAST, For* f, BlockScope* s, Block* parent);
  //Constructor for While loop body
  Block(Parser::While* whileAST, While* w, BlockScope* s, Block* parent);
  //Constructor for dummy block in global scope (for tests)
  Block(BlockScope* s);
  void addStatements(Parser::Block* ast);
  vector<Statement*> stmts;
  //scope of the block
  BlockScope* scope;
  //subroutine whose body contains this block (passed down to child blocks that aren't 
  Subroutine* subr;
  //scope of innermost function enclosing this (passed down to child blocks)
  Scope* funcScope;
  //innermost "breakable" (loop/switch) containing this block (or NULL if none)
  //  (all break statements correspond to this)
  Breakable breakable;
  //innermost loop whose body contains this block (or NULL if none)
  //  (all continue statements correspond to this)
  Loop loop;
  void check();
  void checkPurity(Scope* s);
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
  Assign(Parser::VarAssign* va, Scope* s);
  Assign(Variable* target, Expression* e);
  Assign(Indexed* target, Expression* e);
  Expression* lvalue;
  Expression* rvalue;
  void checkPurity(Scope* s);
  private:
  void commonCtor();  //actually do the semantic checking
};

struct CallStmt : public Statement
{
  //Ctor for when it is known that Expr12 is a call
  CallStmt(Parser::Expr12* call, BlockScope* s);
  //code generator just needs to "evaluate" this expression and discard the result
  CallExpr* eval;
  void checkPurity(Scope* s);
};

struct For : public Statement
{
  //note: scope provided in Parser::For
  For(Parser::For* f, Block* b);
  Block* loopBlock;
  Statement* init;
  Expression* condition;  //check this before each entry to loop body
  Statement* increment;
  void checkPurity(Scope* s);
  private: For() {} //only used inside the real ctor in the for over array case
};

struct While : public Statement
{
  While(Parser::While* w, Block* b);
  Block* loopBlock;
  Expression* condition;
  void checkPurity(Scope* s);
};

struct If : public Statement
{
  If(Parser::If* i, Block* b);
  Expression* condition;
  Statement* body;
  void checkPurity(Scope* s);
};

struct IfElse : public Statement
{
  IfElse(Parser::If* ie, Block* b);
  Expression* condition;
  Statement* trueBody;
  Statement* falseBody;
  void checkPurity(Scope* s);
};

struct Match : public Statement
{
  Match(Parser::Match* m, Block* b);
  Expression* matched;  //the given expression (must be of union type)
  vector<Block*> cases; //correspond 1-1 with matched->type->options
  vector<Variable*> caseVars; //correspond 1-1 with cases
  void checkPurity(Scope* s);
};

struct Switch : public Statement
{
  Switch(Parser::Switch* s, Block* b);
  Expression* switched;
  vector<Expression*> caseValues;
  vector<int> caseLabels; //correspond 1-1 with caseValues
  int defaultPosition;
  //the block that holds all the statements but can't hold any scoped decls
  Block* block;
  void checkPurity(Scope* s);
};

struct Return : public Statement
{
  //Normal constructor
  Return(Parser::Return* r, Block* s);
  //Constructor for void return (only used in Subroutine::check())
  Return(Subroutine* s);
  Expression* value; //can be null (void return)
  Subroutine* from;
  void checkPurity(Scope* s);
};

struct Break : public Statement
{
  //this ctor checks that the statement is being used inside a loop
  Break(Block* s);
  Breakable breakable;
};

struct Continue : public Statement
{
  //this ctor checks that the statement is being used inside a loop or Match
  Continue(Block* s);
  Loop loop;
};

struct Print : public Statement
{
  Print(Parser::PrintNT* p, BlockScope* s);
  vector<Expression*> exprs;
  void checkPurity(Scope* s);
};

struct Assertion : public Statement
{
  Assertion(Parser::Assertion* as, BlockScope* s);
  Expression* asserted;
  void checkPurity(Scope* s);
};

struct Subroutine
{
  //constructor doesn't process the type or body in any way
  Subroutine(Parser::SubroutineNT* snt, Scope* s);
  string name;
  //the full type of this subroutine
  TypeSystem::CallableType* type;
  //Local variables in subroutine scope representing arguments, in order
  vector<Variable*> args;
  Block* body;
  //the scope OF the subroutine, not the one containing it
  SubroutineScope* scope;
  //check both prototype and body
  void check();
};

struct Test
{
  Test(Parser::TestDecl* td, Scope* s);
  Block* run;
  //Remember where the test is declared to make
  //test output useful
  int line;
  int column;
  static vector<Test*> tests;
};

#endif

