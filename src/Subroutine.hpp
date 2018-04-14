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
  Statement(Block* b) : block(b) {}
  Block* block;
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
  Assign(Block* b, Expression* lhs, Expression* rhs);
  void resolve(bool final);
  Expression* lvalue;
  Expression* rvalue;
};

struct CallStmt : public Statement
{
  //Ctor for when it is known that Expr12 is a call
  CallStmt(Block* b, CallExpr* e);
  void resolve(bool final);
  //code generator just needs to "evaluate" this expression and discard the result
  CallExpr* eval;
};

struct For : public Statement
{
  //C-style for loop
  For(Block* b, Statement* init, Expression* condition, Statement* increment, Block* body);
  //for over array
  For(Block* b, vector<string>& tupIter, Expression* arr, Block* body);
  //for over integer range
  For(Block* b, string counter, Expression* begin, Expression* end, Block* body);
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
  While(Block* b, Expression* condition, Block* body);
  void resolve(bool final);
  Expression* condition;
  Block* body;
};

struct If : public Statement
{
  If(Block* b, Expression* condition, Statement* body);
  If(Block* b, Expression* condition, Statement* tbody, Statement* fbody);
  void resolve(bool final);
  Expression* condition;
  Statement* body;
  Statement* elseBody; //null if no else
};

struct Match : public Statement
{
  //Create an empty match statement
  //Add the individual cases after constructing
  Match(Block* b, Expression* m, string varName,
      vector<TypeSystem::Type*>& types,
      vector<Block*>& blocks);
  void resolve(bool final);
  Expression* matched;              //the given expression (must be a union)
  vector<TypeSystem::Type*> types;  //each type must be an option of matched->type
  vector<Block*> cases;             //correspond 1-1 with types
  vector<Variable*> caseVars;       //correspond 1-1 with cases
};

struct Switch : public Statement
{
  //caseIndices and defaultPos are the locations of labels within stmts
  Switch(Block* b, Expression* s,
      vector<int>& caseIndices, vector<Expression*> caseValues,
      vector<Statement*>& stmts, int defaultPos);
  void resolve(bool final);
  Expression* switched;
  vector<Expression*> caseValues;
  vector<int> caseLabels; //correspond 1-1 with caseValues
  int defaultPosition;
  //list of statements - belong to block enclosing the switch
  vector<Statement*> stmts;
};

struct Return : public Statement
{
  //Constructor for returning a value
  Return(Block* b, Expression* value);
  //Constructor for void return
  Return(Block* b);
  void resolve(bool final);
  Expression* value; //null for void return
};

struct Break : public Statement
{
  //this ctor checks that the statement is being used inside a loop
  Break(Block* b);
  void resolve(bool final);
  Breakable breakable;
};

struct Continue : public Statement
{
  //this ctor checks that the statement is being used inside a loop or Match
  Continue(Block* b);
  void resolve(bool final);
  Loop loop;
};

struct Print : public Statement
{
  Print(Block* b, vector<Expression*>& exprs);
  void resolve(bool final);
  vector<Expression*> exprs;
};

struct Assertion : public Statement
{
  Assertion(Block* b, Expression* a);
  void resolve(bool final);
  Expression* asserted;
};

struct Subroutine
{
  //constructor doesn't process the type or body in any way
  Subroutine(Scope* s, string name, TypeSystem::CallableType* ct, vector<string>& argNames, vector<TypeSystem::Type*>& argTypes, Block* body);
  void resolve(bool final);
  string name;
  //the full type of this subroutine
  TypeSystem::CallableType* type;
  //Local variables in subroutine scope representing arguments, in order
  vector<Variable*> args;
  Block* body;
  //scope->node is this
  Scope* scope;
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

