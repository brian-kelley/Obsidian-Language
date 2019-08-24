#ifndef AST_INTERPRETER_H
#define AST_INTERPRETER_H

#include "Common.hpp"
#include "TypeSystem.hpp"
#include "Expression.hpp"
#include "Subroutine.hpp"

struct StackFrame
{
  StackFrame();
  //Local variables are lazily added
  //to this when initialized.
  //
  //If a var is referenced before first initialization,
  //it was used before declaration.
  map<Variable*, Expression*> locals;
  //The return value for this subroutine
  Expression* rv;
};

struct Interpreter
{
  //Interpreter needs to start at entry point subr
  Interpreter(Subroutine* subr, vector<Expression*> args);
  void callSubr(Subroutine* subr, vector<Expression*> args);
  void callExtern(ExternalSubroutine* exSubr, vector<Expression*> args);
  void execute(Statement* stmt);
  //frames.back is the top of the call stack
  vector<StackFrame*> frames;
  map<Variable*, Expression*> globals;
  //Is the topmost function returning?
  bool returning;
};

#endif
