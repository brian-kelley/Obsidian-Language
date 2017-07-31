#include "Subroutine.hpp"

Statement* createStatement(Parser::StatementNT* stmt, BlockScope* bs)
{
  if(stmt->s.is<ScopedDecl*>())
  {
    //local VarDecls are handled in Block ctor, and
    //all other kinds of scoped decls have already been added to scope
    INTERNAL_ERROR;
  }
  else if(stmt->s.is<VarAssign*>())
  {
  }
  else if(stmt->s.is<Print*>())
  {
  }
  else if(stmt->s.is<Call*>())
  {
  }
  else if(stmt->s.is<Block*>())
  {
  }
  else if(stmt->s.is<Return*>())
  {
  }
  else if(stmt->s.is<Continue*>())
  {
  }
  else if(stmt->s.is<Break*>())
  {
  }
  else if(stmt->s.is<Switch*>())
  {
  }
  else if(stmt->s.is<For*>())
  {
  }
  else if(stmt->s.is<While*>())
  {
  }
  else if(stmt->s.is<If*>())
  {
  }
  else if(stmt->s.is<Assertion*>())
  {
  }
  else
  {
    INTERNAL_ERROR;
  }
  return nullptr;
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

