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

namespace IR
{
  struct SubroutineIR;
}

int getNextLambdaID();

extern Subroutine* mainSubr;

struct Statement : public Node
{
  //ctor for statements that don't belong to any block (e.g. subroutine bodies)
  Statement() : block(nullptr) {}
  Statement(Block* b) : block(b) {}
  Block* block;
  virtual void resolveImpl() {}
  virtual ~Statement() {}
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

struct Callable;
struct SubroutineDecl;
struct SubrBase;
struct Subroutine;
struct ExternalSubroutine;

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
};

struct Assign : public Statement
{
  Assign(Block* b, Expression* lhs, Expression* rhs);
  //Update operators (like +=, &=, etc.)
  //Internally, is converted to just lhs := lhs <op> rhs
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
  Expression* matched;         //the given expression (must be a union)
  vector<Type*> types;         //each type must be an option of matched->type
  vector<Block*> cases;        //correspond 1-1 with types
  vector<Variable*> caseVars;  //correspond 1-1 with cases
};

struct Switch : public Statement
{
  Switch(Block* b, Expression* s, Block* block);
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
  Return(Block* b, Expression* v);
  //Constructor for void return
  Return(Block* b);
  void resolveImpl();
  Expression* value;
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
  Scope* usage;
};

struct Assertion : public Statement
{
  Assertion(Block* b, Expression* a);
  void resolveImpl();
  Expression* asserted;
};

//SubroutineDecl represents a set of
//overloaded Callables with the same name,
//and which must be declared together.
struct SubroutineDecl : public Node
{
  SubroutineDecl(string n, Scope* s, bool pure, bool explicitStatic);
  //Find a version matching the arguments.
  //Querying exact match is optional.
  //If *exact, parameter types matched exactly.
  //Otherwise, at least one was converted.
  SubrBase* match(vector<Type*>& params, bool* exact = nullptr);
  //Find a version matching the full CallableType exactly.
  SubrBase* match(CallableType* ct);
  //In many cases, there will only be one version, so return that.
  SubrBase* getOnly()
  {
    if(overloads.size() > 1)
      return nullptr;
    return overloads[0];
  }
  //Resolution resolves every member of the family, but
  //it also checks that no two have identical parameters
  void resolveImpl();
  string name;
  Scope* scope;
  bool isPure;
  StructType* owner;
  vector<SubrBase*> overloads;
};

struct SubrBase : public Node
{
  SubrBase(SubroutineDecl* d)
    : decl(d)
  {}
  virtual void resolveImpl() = 0;
  bool pure()
  {
    return type->pure;
  }
  string name()
  {
    return decl->name;
  }
  StructType* owner()
  {
    return decl->owner;
  }
  SubroutineDecl* decl;
  CallableType* type;
};

struct Subroutine : public SubrBase
{
  //isStatic is just whether there was an explicit "static" before declaration,
  //everything else can be determined from context
  //isPure is whether this is declared as a function
  Subroutine(SubroutineDecl* decl);
  void setSignature(Type* retType, vector<Variable*>& p);
  void resolveImpl();
  //scope that contains the parameters
  Scope* scope;
  //Params are standard local variables in the scope
  vector<Variable*> params;
  //constructor sets owner to innermost struct if there is one and isStatic is false
  //otherwise NULL
  StructType* owner;
  Block* body;
  IR::SubroutineIR* subrIR;
  int id;
};

struct ExternalSubroutine : public SubrBase
{
  ExternalSubroutine(SubroutineDecl* decl, Scope* s, string name, Type* returnType, vector<Type*>& paramTypes, vector<string>& paramNames, vector<bool>& borrow, string& code);
  void resolveImpl();
  SubroutineDecl* decl;
  //the native symbol to call
  string c;
  vector<string> paramNames;
  //How each argument is passed
  vector<bool> paramBorrowed;
  int id;
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

