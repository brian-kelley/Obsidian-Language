#include "Subroutine.hpp"

Statement* createStatement(Parser::StatementNT* stmt, BlockScope* bs)
{
  if(stmt->s.is<ScopedDecl*>())
  {
    //only process this here if it is a VarDecl
    //all other kinds of scoped decl have already been processed
    auto sd = stmt->s.get<ScopedDecl*>();
    if(sd->decl.is<VarDecl*>())
    {
      return new NewVar(sd->decl.get<VarDecl*>(), bs);
    }
  }

      ScopedDecl*,
      VarAssign*,
      Print*,
      ExpressionNT*,
      Block*,
      Return*,
      Continue*,
      Break*,
      Switch*,
      For*,
      While*,
      If*,
      Assertion*,
}

Block::Block(Parser::Block* b, Scope* s)
{
  for(auto stmt : b->statements)
  {
    stmts.push_back(createStatement(stmt, s);
  }
}

NewVar::NewVar(Parser::VarDecl* vd, Scope* s)
{
  s->variables.push_back(new Variable(s, vd));
}

Function::Function(Parser::FuncDef* a, Scope* enclosing)
{
}

Procedure::Procedure(Parser::ProcDef* a, Scope* enclosing)
{
}

