#ifndef SUBROUTINE_H
#define SUBROUTINE_H

#include "TypeSystem.hpp"
#include "Parser.hpp"
#include "Expression.hpp"
#include "Scope.hpp"
#include "AST.hpp"

/***************************************************************************/
// Subroutine: middle-end structures for program behavior and control flow //
/***************************************************************************/

struct Statement : public Node
{
  //normal ctor: automatically set index within parent block
  Statement()
  {
    resolved = false;
  }
  virtual void resolve(bool final);
  virtual ~Statement() {}
};

//Statement types
struct Block;
//LocalVar needs to be a kind of statement with a position in block,
//because locals must be declared before use
struct LocalVar;
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

struct Test;

//Loop (anything that can have continue statement)
typedef variant<None, For*, While*> Loop;
//Breakable (anything that can have break statement)
typedef variant<None, For*, While*, Switch*> Breakable;

//Block: list of statements
struct Block : public Statement
{
  //Constructor for function/procedure body
  Block(Subroutine* subr);
  //Constructor for empty block
  Block(Block* parent);
  //Constructor for For loop body
  Block(For* f);
  //Constructor for While loop body
  Block(While* w);
  void resolve(bool final);
  vector<Statement*> stmts;
  //scope of the block
  Scope* scope;
  //subroutine whose body contains this block (passed down to child blocks)
  Subroutine* subr;
  //innermost "breakable" (loop/switch) containing this block
  Breakable breakable;
  //innermost loop whose body is or contains this
  Loop loop;
  int statementCount;
};

//Create any kind of Statement - adds to block
Statement* createStatement(Block* s, Parser::StatementNT* stmt);

struct Assign : public Statement
{
  Assign(Expression* lhs, Expression* rhs);
  void resolve(bool final);
  Expression* lvalue;
  Expression* rvalue;
};

struct CallStmt : public Statement
{
  //Ctor for when it is known that Expr12 is a call
  CallStmt(CallExpr* e);
  void resolve(bool final);
  //code generator just needs to "evaluate" this expression and discard the result
  CallExpr* eval;
};

struct For : public Statement
{
  //C-style for loop
  For(Statement* init, Expression* condition, Statement* increment, Block* body);
  //for over array
  For(vector<string>& tupIter, Expression* arr, Block* body);
  //for over integer range
  For(string counter, Expression* begin, Expression* end, Block* body);
  void resolve(bool final);
  Statement* init;
  Expression* condition;
  Statement* increment;
  Block* body;
  //for a multidimensional for over array, bodyImpl is the outermost block,
  //while body is the innermost
  private:
  //create init/condition/increment to iterate over integer range
  //used by resolve() for both array and range constructors
  //this creates a variable with name counter and type compatible with begin/end
  //
  //precondition: begin and end must be resolved and integers
  Variable* setupRange(string counter, Expression* begin, Expression* end);
};

struct While : public Statement
{
  While(Expression* condition, Block* body);
  void resolve(bool final);
  Expression* condition;
  Block* body;
};

struct If : public Statement
{
  If(Expression* condition, Statement* body);
  If(Expression* condition, Statement* tbody, Statement* fbody);
  void resolve(bool final);
  Expression* condition;
  Statement* body;
  Statement* elseBody; //null if no else
};

struct IfElse : public Statement
{
  IfElse(Parser::If* ie, Block* b);
  Expression* condition;
  Statement* trueBody;
  Statement* falseBody;
};

struct Match : public Statement
{
  Expression* matched;  //the given expression (must be of union type)
  vector<TypeSystem::Type*> types;
  vector<Block*> cases; //correspond 1-1 with matched->type->options
  vector<Variable*> caseVars; //correspond 1-1 with cases
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
};

struct Return : public Statement
{
  //Normal constructor
  Return(Parser::Return* r, Block* s);
  //Constructor for void return (only used in Subroutine::check())
  Return(Subroutine* s);
  Expression* value; //can be null (void return)
  Subroutine* from;
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
};

struct Assertion : public Statement
{
  Assertion(Parser::Assertion* as, BlockScope* s);
  Expression* asserted;
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
  //scope->node is this
  Scope* scope;
  //check both prototype and body
  void check();
};

struct ExternalSubroutine
{
  ExternalSubroutine(Parser::ExternSubroutineNT*, Scope* s);
  TypeSystem::CallableType* type;
  //the C code that provides the body of this subroutine
  string c;
};

struct Test : public Node
{
  Test(Block* b, Scope* s);
  Block* run;
  static vector<Test*> tests;
};

#endif

