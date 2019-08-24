#include "AstInterpreter.hpp"

StackFrame::StackFrame()
{
  rv = nullptr;
}

Interpreter::Interpreter(Subroutine* subr, vector<Expression*> args)
{
  returning = false;
  callSubr(subr, args);
}

Expression* Interpreter::callSubr(Subroutine* subr, vector<Expression*> args)
{
  returning = false;
  //push stack frame
  frames.emplace_back();
  //Execute statements in linear sequence.
  //If a return is encountered, execute() returns and
  //the return value will be placed in the frame rv
  for(auto s : subr->stmts)
  {
    execute(s);
    if(returning)
    {
      Expression* rv = frames.back().rv;
      frames.pop_back();
      return rv;
    }
  }
  frames.pop_back();
  //implicit return, no return value
  return nullptr;
}

Expression* Interpreter::callExtern(ExternalSubroutine* exSubr, vector<Expression*> args)
{
}

void execute(Statement* stmt)
{

}

