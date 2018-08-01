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

extern bool programHasMain;

struct Statement : public Node
{
  //ctor for statements that don't belong to any block (e.g. subroutine bodies)
  Statement() : block(nullptr) {}
  //normal ctor: automatically set index within parent block
  Statement(Block* b) : block(b) {}
  Block* block;
  virtual void resolveImpl() {}
  virtual ~Statement() {}
  int statementIndex;
};

//Statement types
struct Block;
struct Assign;
struct CallStmt;
struct For;
struct ForC;
struct ForRange;
struct ForArray;
struct While;
struct If;
struct Return;
struct Break;
struct Continue;
struct Print;
struct Assertion;
struct Switch;
struct Match;

struct Subroutine;

struct Test;

//Loop (anything that can have continue statement)
typedef variant<None, For*, While*> Loop;
//Breakable (anything that can have break statement)
typedef variant<None, For*, While*, Switch*> Breakable;

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
  //Constructor for standalone block (for tests)
  Block(Scope* s);
  void resolveImpl();
  void addStatement(Statement* s);
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

struct Assign : public Statement
{
  Assign(Block* b, Expression* lhs, Expression* rhs);
  Assign(Block* b, Expression* lhs, int op, Expression* rhs = nullptr);
  void resolveImpl();
  Expression* lvalue;
  Expression* rvalue;
};

struct CallStmt : public Statement
{
  CallStmt(Block* b, CallExpr* e);
  void resolveImpl();
  //code generator evaluates eval
  //and discards returned value, if any
  CallExpr* eval;
};

struct For : public Statement
{
  For(Block* b);
  //Outer (exists just for the scope) contains
  //counters and intialization/increment statements
  Block* outer;
  //Inner block is actually executed each iteration
  //(contains user statements)
  //Loop/Breakable of inner are this loop
  Block* inner;
  virtual void resolveImpl() = 0;
};

//C-style for loop:
struct ForC : public For
{
  //note: init, condition and increment are optional (can be NULL)
  ForC(Block* b);
  void resolveImpl();
  //Parser directly assigns these members:
  Statement* init;
  Expression* condition;
  Statement* increment;
};

struct ForArray : public For
{
  ForArray(Block* b);
  void createIterators(vector<string>& iters);
  void resolveImpl();
  //create an empty inner body (parser puts user's statements here)
  Block* createInnerBody();
  //the (long) counters that count from 0 to each dimension of the array
  vector<Variable*> counters;
  Expression* arr;
  Variable* iter;
};

struct ForRange : public For
{
  ForRange(Block* b, string counterName, Expression* begin, Expression* end);
  Variable* counter;
  Expression* begin;
  Expression* end;
  void resolveImpl();
};

struct While : public Statement
{
  //body can be any statement,
  //but internally body must be a block (for Loop/Breakable)
  While(Block* b, Expression* condition);
  void resolveImpl();
  Expression* condition;
  Block* body;
};

struct If : public Statement
{
  If(Block* b, Expression* condition, Statement* body);
  If(Block* b, Expression* condition, Statement* tbody, Statement* fbody);
  void resolveImpl();
  Expression* condition;
  Statement* body;
  Statement* elseBody; //null if no else
};

struct Match : public Statement
{
  //Create an empty match statement
  //Add the individual cases after constructing
  Match(Block* b, Expression* m, string varName,
      vector<Type*>& types,
      vector<Block*>& blocks);
  void resolveImpl();
  Expression* matched;              //the given expression (must be a union)
  vector<Type*> types;  //each type must be an option of matched->type
  vector<Block*> cases;             //correspond 1-1 with types
  vector<Variable*> caseVars;       //correspond 1-1 with cases
};

struct Switch : public Statement
{
  //caseIndices and defaultPos are the locations of labels within stmts
  Switch(Block* b, Expression* s,
      vector<int>& caseIndices, vector<Expression*> caseValues,
      int defaultPos, Block* block);
  void resolveImpl();
  Expression* switched;
  vector<Expression*> caseValues;
  vector<int> caseLabels; //correspond 1-1 with caseValues
  int defaultPosition;
  Block* block;
};

struct Return : public Statement
{
  //Constructor for returning a value
  Return(Block* b, Expression* value);
  //Constructor for void return
  Return(Block* b);
  void resolveImpl();
  Expression* value; //null for void return
};

struct Break : public Statement
{
  //this ctor checks that the statement is being used inside a loop
  Break(Block* b);
  void resolveImpl();
  Breakable breakable;
};

struct Continue : public Statement
{
  //this ctor checks that the statement is being used inside a loop or Match
  Continue(Block* b);
  void resolveImpl();
  Loop loop;
};

struct Print : public Statement
{
  Print(Block* b, vector<Expression*>& exprs);
  void resolveImpl();
  vector<Expression*> exprs;
};

struct Assertion : public Statement
{
  Assertion(Block* b, Expression* a);
  void resolveImpl();
  Expression* asserted;
};

struct Subroutine : public Node
{
  //isStatic is just whether there was an explicit "static" before declaration,
  //everything else can be determined from context
  //isPure is whether this is declared as a function
  Subroutine(Scope* s, string name, bool isStatic, bool pure, Type* returnType, vector<string>& argNames, vector<Type*>& argTypes);
  void resolveImpl();
  string name;
  //the full type of this subroutine
  CallableType* type;
  //Local variables in subroutine scope representing arguments, in order
  vector<Variable*> args;
  //constructor sets owner to innermost struct if there is one and isStatic is false
  //otherwise NULL
  StructType* owner;
  Block* body;
  //scope->node is this
  Scope* scope;
};

struct ExternalSubroutine : public Node
{
  ExternalSubroutine(Scope* s, string name, Type* returnType, vector<Type*>& argTypes, vector<string>& argNames, string& code);
  string name;
  void resolveImpl();
  CallableType* type;
  //the C code that provides the body of this subroutine
  string c;
  Scope* scope;
  vector<string> argNames;
};

struct Test : public Node
{
  Test(Scope* s, Block* b);
  void resolveImpl();
  //scope needed to resolve run
  Scope* scope;
  Block* run;
  //since tests don't live in the program,
  //keep a list of all tests
  static vector<Test*> tests;
};

#endif

