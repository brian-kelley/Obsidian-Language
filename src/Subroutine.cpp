#include "Subroutine.hpp"

Block::Block(Parser::Block* b, Scope* s)
{
  bs = dynamic_cast<BlockScope*>(s);
  for(auto stmt : b->statements)
  {
    if(stmt->s.is<ScopedDecl*>())
    {
      auto sd = stmt->s.is<ScopedDecl*>();
      if(sd->decl.is<VarDecl*>())
      {
        auto vd = sd->decl.get<VarDecl*>();
        addLocalVariable(this, vd);
      }
    }
    stmts.push_back(createStatement(s, stmt);
  }
}

Statement* createStatement(Block* b, Parser::StatementNT* stmt)
{
  auto scope = b->scope;
  if(stmt->s.is<Parser::VarAssign*>())
  {
    return new Assign(stmt->s.get<VarAssign*>(), scope);
  }
  else if(stmt->s.is<Parser::PrintNT*>())
  {
    return new Print(stmt->s.get<Parser::PrintNT*>(), scope);
  }
  else if(stmt->s.is<Parser::CallNT*>())
  {
    return new CallStmt(stmt->s.get<Parser::CallNT*>(), scope);
  }
  else if(stmt->s.is<Block*>())
  {
    return new Block(stmt->s.get<Block*>(), scope);
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
}

void addLocalVariable(Block* b, Parser::VarDecl* vd)
{
  //Make sure variable doesn't already exist (shadowing var is error)
  Parser::Member search;
  search.ident = vd->name;
  if(s->findVariable(&search))
  {
    errAndQuit(string("variable \"") + vd->name + "\" already exists");
  }
  Variable* newVar = new Variable(s, vd);
  s->vars.push_back(newVar);
  if(vd->val)
  {
    b->stmts.push_back(new Assign(newVar, getExpression(s, vd->val)));
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

CallStmt::CallStmt(Parser::CallNT* c, BlockScope* s)
{
}

For::For(Parser::For* f, Scope* s)
{
  auto loopScope = f->scope;
  if(f->f.is<Parser::ForC*>())
  {
    auto fc = f->f.get<Parser::ForC*>();
    //if there is an initializer statement, add it to the block as the first statement
    if(fc->decl)
    {
    }
  }
  else if(f->f.is<Parser::ForRange1*>())
  {
    //introduce integer counter i/j/k/etc
  }
  else if(f->f.is<Parser::ForRange2*>())
  {
  }
  else if(f->f.is<Parser::ForArray*>())
  {
  }
  /*
   * Fields:
  Type* counterType;
  Expression* start;
  Expression* condition;
  Statement* increment;
  Statement* body;
  */
}

Function::Function(Parser::FuncDef* a, Scope* enclosing)
{
}

Procedure::Procedure(Parser::ProcDef* a, Scope* enclosing)
{
}

