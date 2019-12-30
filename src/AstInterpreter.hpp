#ifndef AST_INTERPRETER_H
#define AST_INTERPRETER_H

#include "Common.hpp"
#include "TypeSystem.hpp"
#include "Expression.hpp"
#include "Subroutine.hpp"

struct StackFrame
{
  StackFrame()
  {
    thisExprLval = nullptr;
    thisExprRval = nullptr;
  }
  StackFrame(Expression*&& e)
  {
    thisExprLval = nullptr;
    thisExprRval = e;
  }
  StackFrame(Expression*& e)
  {
    thisExprLval = &e;
    thisExprRval = nullptr;
  }
  Expression*& getThis()
  {
    if(thisExprLval)
      return *thisExprLval;
    else if(!thisExprRval)
      INTERNAL_ERROR;
    return thisExprRval;
  }
  //Local variables are lazily added
  //to this when initialized.
  map<Variable*, Expression*> locals;
  Expression** thisExprLval;
  Expression* thisExprRval;
};

struct Interpreter
{
  //Interpreter needs to start at entry point subr
  Interpreter(Subroutine* subr, vector<Expression*> args);
  //thisExpr is a reference, not a value!
  //Any modifications to it through a method apply to the original, not a copy.
  Expression* callSubr(Subroutine* subr, vector<Expression*> args);
  Expression* callSubr(Subroutine* subr, vector<Expression*> args, Expression*& thisExpr);
  Expression* callSubr(Subroutine* subr, vector<Expression*> args, Expression*&& thisExpr);
  Expression* callExtern(ExternalSubroutine* exSubr, vector<Expression*> args);
  void execute(Statement* stmt);
  Expression* evaluate(Expression* e);
  Expression*& evaluateLValue(Expression* e);
  static CompoundLiteral* createArray(uint64_t* dims, int ndims, Type* elem);
  static Expression* convertConstant(Expression* value, Type* type);
  void assignVar(Variable* v, Expression* e);
  Expression*& readVar(Variable* v);
  //frames.back is the top of the call stack
  stack<StackFrame> frames;
  map<Variable*, Expression*> globals;
  //Is the topmost function returning?
  bool returning;
  bool breaking;
  bool continuing;
  //The return value for the current function
  Expression* rv;
private:
  Expression* invoke(Subroutine* subr, vector<Expression*>& args);
};

#endif

