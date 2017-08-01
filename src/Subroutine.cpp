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
    if(stmt->s.is<ScopedDecl*>())
    {
      auto sd = stmt->s.is<ScopedDecl*>();
      if(sd->decl.is<VarDecl*>())
      {
        auto vd = sd->decl.get<VarDecl*>();
        Variable* newVar = new Variable(s, vd);
        s->vars.push_back(newVar);
        //if the new variable has an initializing expression,
        //treat as a separate Assign statement
        if(vd->val)
        {
          stmts.push_back(new Assign(newVar, getExpression(s, vd->val)));
        }
      }
    }
    stmts.push_back(createStatement(stmt, s);
  }
}

Assign::Assign(Parser::VarAssign* va, Scope* s)
{
  lvalue = va->target;
  rvalue = va->rhs;
  //make sure that lvalue is in fact an lvalue, and that
  //rvalue can convert to lvalue's type
  if(!lvalue->assignable())
  {
    errAndQuit("cannot assign to that expression");
  }
  if(!lvalue->type)
  {
    INTERNAL_ERROR;
  }
  if(!lvalue->type->canConvert(rvalue))
  {
    errAndQuit("incompatible types for assignment");
  }
}

Assign::Assign(Variable* target, Expression* e, Scope* s)
{
  lvalue = new VarExpr(s, target);
  //vars are always lvalues, no need to check that
  if(!lvalue->type->canConvert(e))
  {
    errAndQuit("incompatible types for assignment");
  }
  rvalue = e;
}

For::For(Parser::For* f, Scope* s)
{
  if(f->f.is<Parser::ForC*>())
  {
    auto fc = f->f.get<Parser::ForC*>();
  }
  else if(f->f.is<Parser::ForRange1*>())
  {
  }
  else if(f->f.is<Parser::ForRange2*>())
  {
  }
  else if(f->f.is<Parser::ForArray*>())
  {
  }
  Type* counterType;
  Expression* start;
  Expression* condition;
  Statement* increment;
  Statement* body;
}

Function::Function(Parser::FuncDef* a, Scope* enclosing)
{
}

Procedure::Procedure(Parser::ProcDef* a, Scope* enclosing)
{
}

