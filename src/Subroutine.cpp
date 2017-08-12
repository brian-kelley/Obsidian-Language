#include "Subroutine.hpp"

Block::Block(Parser::Block* b, Subroutine* subr) : scope(subr->scope), ast(b)
{
  addStatements();
}

Block::Block(Parser::Block* b, Block* parent) : scope(b->bs), ast(b)
{
  addStatements();
}

Block::Block(BlockScope* bs) : scope(bs), ast(nullptr) {}
//don't add any statements yet

void Block::addStatements()
{
  for(auto stmt : ast->statements)
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
    stmts.push_back(createStatement(s, stmt));
  }
}

Statement* createStatement(Block* b, Parser::StatementNT* stmt)
{
  auto scope = b->scope;
  if(stmt->s.is<Parser::ScopedDecl*>())
  {
    //only scoped decl to handle now is VarDecl
    auto sd = stmt->s.get<Parser::ScopedDecl*>();
    if(sd->s.is<Parser::VarDecl*>())
    {
      addLocalVariable(b, sd->s.get<Parser::VarDecl*>());
    }
  }
  else if(stmt->s.is<Parser::VarAssign*>())
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
  else if(stmt->s.is<Parser::Block*>())
  {
    return new Block(stmt->s.get<Block*>(), scope);
  }
  else if(stmt->s.is<Parser::Return*>())
  {
    return new Return(stmt->s.get<Parser::Return*>(), scope);
  }
  else if(stmt->s.is<Parser::Continue*>())
  {
    return new Continue;
  }
  else if(stmt->s.is<Parser::Break*>())
  {
    return new Break;
  }
  else if(stmt->s.is<Parser::Switch*>())
  {
    ERR_MSG("switch statements aren't supported (yet)");
  }
  else if(stmt->s.is<Parser::For*>())
  {
    return new For(stmt->s.get<Parser::For*>(), scope);
  }
  else if(stmt->s.is<Parser::While*>())
  {
    return new While(stmt->s.get<Parser::While*>(), scope);
  }
  else if(stmt->s.is<Parser::If*>())
  {
    return new While(stmt->s.get<Parser::While*>(), scope);
  }
  else if(stmt->s.is<Parser::Assertion*>())
  {
    return new Assertion(stmt->s.get<Parser::Assertion*>(), scope);
  }
}

void addLocalVariable(Block* b, Parser::VarDecl* vd)
{
  //Make sure variable doesn't already exist (shadowing var is error)
  Parser::Member search;
  search.ident = vd->name;
  if(s->findVariable(&search))
  {
    ERR_MSG(string("variable \"") + vd->name + "\" already exists");
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
    ERR_MSG("cannot assign to that expression");
  }
  if(!lvalue->type)
  {
    INTERNAL_ERROR;
  }
  if(!lvalue->type->canConvert(rvalue))
  {
    ERR_MSG("incompatible types for assignment");
  }
}

Assign::Assign(Variable* target, Expression* e, Scope* s)
{
  lvalue = new VarExpr(s, target);
  //vars are always lvalues, no need to check that
  if(!lvalue->type->canConvert(e))
  {
    ERR_MSG("incompatible types for assignment");
  }
  rvalue = e;
}

CallStmt::CallStmt(Parser::CallNT* c, BlockScope* s)
{
  //look up callable (make sure it is a procedure, not a function)
  called = s->findSubroutine(c->callable);
  if(!called)
  {
    ERR_MSG("tried to call undeclared procedure \"" << c->callable << "\" with " << c->args.size() << " arguments");
  }
  for(auto it : c->args)
  {
    args.push_back(getExpression(s, it));
  }
}

For::For(Parser::For* f, Scope* s)
{
  auto loopScope = f->scope;
  loopBlock = new Block(loopScope);
  if(f->f.is<Parser::ForC*>())
  {
    auto fc = f->f.get<Parser::ForC*>();
    //if there is an initializer statement, add it to the block as the first statement
    if(fc->decl)
    {
      //if fc->decl is a ScopedDecl/VarDecl,
      //this will create the counter as local variable in loopScope
      init = createStatement(loopBlock, fc->decl);
    }
    if(fc->condition)
    {
      condition = getExpression(loopScope, fc->condition);
      if(condition->type != TypeSystem::primitives[TypeNT::BOOL])
      {
        ERR_MSG("condition expression in C-style for loop must be a boolean");
      }
    }
    if(fc->incr)
    {
      increment = createStatement(loopBlock, fc->incr);
    }
  }
  else if(f->f.is<Parser::ForRange1*>())
  {
    //introduce integer counter
    //counter var names start at i and go to z, after that there is error
  }
  else if(f->f.is<Parser::ForRange2*>())
  {
  }
  else if(f->f.is<Parser::ForArray*>())
  {
  }
  //body is required by grammar
  body = createStatement(loopBlock, f->body);
}

While::While(Parser::While* w, BlockScope* s)
{
}

If::If(Parser::If* i, BlockScope* s)
{
}

IfElse::IfElse(Parser::IfElse* ie, BlockScope* s)
{
}

Return::Return(Parser::Return* r, BlockScope* s)
{
}

Break::Break()
{
}

Continue::Continue()
{
}

Function::Function(Parser::FuncDef* a, Scope* enclosing)
{
}

Procedure::Procedure(Parser::ProcDef* a, Scope* enclosing)
{
}

