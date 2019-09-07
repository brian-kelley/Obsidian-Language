#ifndef AST_INTERPRETER_H
#define AST_INTERPRETER_H

#include "Common.hpp"
#include "TypeSystem.hpp"
#include "Expression.hpp"
#include "Subroutine.hpp"

struct StackFrame
{
  //Local variables are lazily added
  //to this when initialized.
  //
  //If a var is referenced before first initialization,
  //it was used before declaration.
  map<Variable*, Expression*> locals;
  Expression* thisExpr;
};

struct Interpreter
{
  //Interpreter needs to start at entry point subr
  Interpreter(Subroutine* subr, vector<Expression*> args);
  //thisExpr is a reference, not a value!
  //Any modifications to it through a method apply to the original, not a copy.
  Expression* callSubr(Subroutine* subr, vector<Expression*> args, Expression* thisExpr = nullptr);
  Expression* callExtern(ExternalSubroutine* exSubr, vector<Expression*> args);
  void execute(Statement* stmt);
  Expression* evaluate(Expression* e);
  Expression*& evaluateLVal(Expression* e);
  CompoundLiteral* createArray(uint64_t* dims, int ndims, Type* elem);
  Expression* convertConstant(Expression* value, Type* type);
  void assignVar(Variable* v, Expression* e);
  Expression* readVar(Variable* v);
  //frames.back is the top of the call stack
  vector<StackFrame> frames;
  map<Variable*, Expression*> globals;
  //Is the topmost function returning?
  bool returning;
  bool breaking;
  bool continuing;
  //The return value for the current function
  Expression* rv;
};

#endif
