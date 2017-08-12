#include "Subroutine.hpp"

Block::Block(Parser::Block* b, Subroutine* subr) : scope(subr->scope), ast(b)
{
  this->subr = subr;
  this->loop = nullptr;
  addStatements();
}

Block::Block(Parser::Block* b, Block* parent) : scope(b->bs), ast(b)
{
  this->subr = parent->subr;
  this->loop = parent->loop;
  addStatements();
}

void Block::addStatements()
{
  for(auto stmt : ast->statements)
  {
    if(stmt->s.is<ScopedDecl*>())
    {
      auto sd = stmt->s.get<ScopedDecl*>();
      if(sd->decl.is<VarDecl*>())
      {
        auto vd = sd->decl.get<VarDecl*>();
        addLocalVariable(scope, vd);
      }
      else if(sd->decl.is<FuncDef*>())
      {
        scope->subr.push_back(new Function(sd->decl.get<FuncDef*>(), scope));
      }
      else if(sd->decl.is<ProcDef*>())
      {
        scope->subr.push_back(new Procedure(sd->decl.get<ProcDef*>(), scope));
      }
    }
    stmts.push_back(createStatement(scope, stmt));
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
      return addLocalVariable(b, sd->s.get<Parser::VarDecl*>());
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
    return new Block(stmt->s.get<Block*>(), b);
  }
  else if(stmt->s.is<Parser::Return*>())
  {
    return new Return(stmt->s.get<Parser::Return*>(), b);
  }
  else if(stmt->s.is<Parser::Continue*>())
  {
    return new Continue(b);
  }
  else if(stmt->s.is<Parser::Break*>())
  {
    return new Break(b);
  }
  else if(stmt->s.is<Parser::Switch*>())
  {
    ERR_MSG("switch statements aren't supported (yet)");
  }
  else if(stmt->s.is<Parser::For*>())
  {
    return new For(stmt->s.get<Parser::For*>(), b);
  }
  else if(stmt->s.is<Parser::While*>())
  {
    return new While(stmt->s.get<Parser::While*>(), b);
  }
  else if(stmt->s.is<Parser::If*>())
  {
    auto i = stmt->s.get<Parser::If*>();
    if(i->elseBody)
    {
      return new IfElse(i, b);
    }
    else
    {
      return new If(i, b);
    }
  }
  else if(stmt->s.is<Parser::Assertion*>())
  {
    return new Assertion(stmt->s.get<Parser::Assertion*>(), scope);
  }
}

Statement* addLocalVariable(BlockScope* s, Parser::VarDecl* vd)
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
    return new Assign(newVar, getExpression(s, vd->val));
  }
  return nullptr;
}

Statement* addLocalVariable(BlockScope* s, string name, Type* type, Expression* init)
{
  //Make sure variable doesn't already exist (shadowing var is error)
  Parser::Member search;
  search.ident = vd->name;
  if(s->findVariable(&search))
  {
    ERR_MSG(string("variable \"") + vd->name + "\" already exists");
  }
  Variable* newVar = new Variable(s, name, type);
  s->vars.push_back(newVar);
  if(vd->val)
  {
    return new Assign(newVar, init);
  }
  return nullptr;
}

Assign::Assign(Parser::VarAssign* va, BlockScope* s)
{
  lvalue = getExpression(s, va->target);
  rvalue = getExpression(s, va->rhs);
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
  Subroutine* subr = s->findSubroutine(c->callable);
  if(dynamic_cast<Function*>(subr))
  {
    ERR_MSG("called function " << subr->name << " without using its return value - should it be a procedure?");
  }
  called = dynamic_cast<Procedure*>(subr);
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
  Expression* one = new IntLiteral(1);
  if(f->f.is<Parser::ForC*>())
  {
    auto fc = f->f.get<Parser::ForC*>();
    //if there is an initializer statement, add it to the block as the first statement
    if(fc->decl)
    {
      //if fc->decl is a ScopedDecl/VarDecl,
      //this will create the counter as local variable in loopScope
      init = createStatement(loopScope, fc->decl);
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
      increment = createStatement(loopScope, fc->incr);
    }
  }
  else if(f->f.is<Parser::ForRange1*>() || f->f.is<Parser::ForRange2*>())
  {
    //introduce counter (counter type must be integer and is determined from the upper bound expression)
    //counter var names start at i and go to z, after that there is error
    Expression* lowerBound;
    Expression* upperBound;
    if(f->f.is<Parser::ForRange1*>())
    {
      auto f1 = f->f.get<Parser::ForRange1*>();
      lowerBound = new IntLiteral(0);
      upperBound = getExpression(loopScope, f1->expr);
    }
    else
    {
      auto f2 = f->f.get<Parser::ForRange1*>();
      lowerBound = getExpression(loopScope, f2->start);
      upperBound = getExpression(loopScope, f2->end);
    }
    if(upperBound->type == nullptr || !upperBound->type->isInteger())
    {
      errAndQuit("0..n and a..b ranged for loops require lower and upper bounds to be some integer type");
    }
    bool foundCounterName = false;
    char nameChar;
    for(nameChar = 'i'; nameChar <= 'z'; nameChar++)
    {
      Parser::Member mem;
      mem.ident = string("") + nameChar;
      if(!s->findVariable(&mem))
      {
        //this variable doesn't already exist, so use this name
        foundCounterName = true;
        break;
      }
    }
    if(!foundCounterName)
    {
      ERR_MSG("variables i-z already exist so can't use ranged for (use C-style loop with different name)");
    }
    string counterName = "";
    counterName += nameChar;
    init = addLocalVariable(loopScope, counterName, upperBound->type, lowerBound);
    Parser::Member finalCountMem;
    finalCountMem->ident = counterName;
    Variable* counter = loopScope->findVariable(&finalCountMem);
    auto counterExpr = new VarExpr(loopScope, counter);
    //the condition is counterExpr < upperBound
    condition = new BinaryArith(counterExpr, CMPL, upperBound);
    //the increment is counter = counter + one
    increment = new Assign(counter, new BinaryArith(counter, PLUS, one), loopScope);
  }
  else if(f->f.is<Parser::ForArray*>())
  {
    ERR_MSG("for loop over array isn't suppoted (yet)...");
  }
  //body is required by grammar
  body = createStatement(loopScope, f->body);
}

While::While(Parser::While* w, BlockScope* s)
{
  condition = getExpression(s, w->cond);
  if(condition->type != TypeSystem::primitives[TypeNT::BOOL])
  {
    ERR_MSG("while loop condition must be a bool");
  }
  body = createStatement(loopScope, w->body);
}

If::If(Parser::If* i, BlockScope* s)
{
  condition = getExpression(s, i->cond);
  if(condition->type != TypeSystem::primitives[TypeNT::BOOL])
  {
    ERR_MSG("while loop condition must be a bool");
  }
  body = createStatement(loopScope, i->body);
}

IfElse::IfElse(Parser::IfElse* ie, Block* b)
{
  condition = getExpression(s->scope, i->cond);
  if(condition->type != TypeSystem::primitives[TypeNT::BOOL])
  {
    ERR_MSG("while loop condition must be a bool");
  }
  trueBody = createStatement(b, i->ifBody);
  falseBody = createStatement(b, i->elseBody);
}

Return::Return(Parser::Return* r, Block* b)
{
  value = nullptr;
  bool voidReturn = r->ex == nullptr;
  if(!voidReturn)
  {
    value = getExpression(b->scope, r->ex);
  }
  //Make sure that the return expression has a type that matches the subroutine's retType
  if(voidReturn && subr->retType != TypeSystem::primitives[TypeNT::BOOL])
  {
    ERR_MSG("function or procedure doesn't return void, so return must have an expression");
  }
  if(!voidReturn && subr->retType == TypeSystem::primitives[TypeNT::BOOL])
  {
    ERR_MSG("procedure returns void but a return expression was provided");
  }
  if(!voidReturn && !subr->retType->canConvert(value))
  {
    ERR_MSG("returned expression can't be converted to the function/procedure return type");
  }
}

Break::Break(Block* b)
{
  //make sure the break is inside a loop
  if(!b->loop)
  {
    ERR_MSG("break statement used outside of a for or while loop");
  }
  loop = b->loop;
}

Continue::Continue(Block* b)
{
  //make sure the continue is inside a loop
  if(!b->loop)
  {
    ERR_MSG("continue statement used outside of a for or while loop");
  }
  loop = b->loop;
}

Subroutine::Subroutine(Scope* enclosing, Parser::Block* block) : scope(block->bs)
{
}

Function::Function(Parser::FuncDef* a, Scope* enclosing) : Subroutine(, Parser::Block* block) : s(enclosing) {}
{
  retType = getType(a->type->retType, enclosing, nullptr);
  for(auto it : a->type->args)
  {
  }
  scope = a->body->bs;
  pure = true;
}

Procedure::Procedure(Parser::ProcDef* a, Scope* enclosing)
{
  retType = getType(a->type->retType, enclosing, nullptr);
  scope = a->body->bs;
  pure = false;
}

