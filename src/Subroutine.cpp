#include "Subroutine.hpp"
#include "Variable.hpp"

Block::Block(Parser::Block* b, Subroutine* sub) : ast(b), scope(b->bs)
{
  this->subr = sub;
  this->loop = nullptr;
  addStatements();
}

Block::Block(Parser::Block* b, Block* parent) : ast(b), scope(b->bs)
{
  this->subr = parent->subr;
  this->loop = parent->loop;
  addStatements();
}

Block::Block(Parser::For* lp, For* f, Block* parent) : ast(lp->body), scope(lp->body->bs)
{
  this->subr = parent->subr;
  this->loop = new Loop(f);
}

Block::Block(Parser::While* lp, While* w, Block* parent) : ast(lp->body), scope(lp->body->bs)
{
  this->subr = parent->subr;
  this->loop = new Loop(w);
  addStatements();
}

void Block::addStatements()
{
  for(auto stmt : ast->statements)
  {
    if(stmt->s.is<Parser::ScopedDecl*>())
    {
      auto sd = stmt->s.get<Parser::ScopedDecl*>();
      if(sd->decl.is<Parser::FuncDef*>())
      {
        scope->subr.push_back(new Function(sd->decl.get<Parser::FuncDef*>()));
      }
      else if(sd->decl.is<Parser::ProcDef*>())
      {
        scope->subr.push_back(new Procedure(sd->decl.get<Parser::ProcDef*>()));
      }
    }
    stmts.push_back(createStatement(this, stmt));
  }
}

Statement* createStatement(Block* b, Parser::StatementNT* stmt)
{
  auto scope = b->scope;
  if(stmt->s.is<Parser::ScopedDecl*>())
  {
    //only scoped decl to handle now is VarDecl
    auto sd = stmt->s.get<Parser::ScopedDecl*>();
    if(sd->decl.is<Parser::VarDecl*>())
    {
      return addLocalVariable(b->scope, sd->decl.get<Parser::VarDecl*>());
    }
  }
  else if(stmt->s.is<Parser::VarAssign*>())
  {
    return new Assign(stmt->s.get<Parser::VarAssign*>(), scope);
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
    return new Block(stmt->s.get<Parser::Block*>(), b);
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
  else if(stmt->s.is<Parser::EmptyStatement*>())
  {
  }
  cout << "About to internal error, actual Parser Statement tag: " << stmt->s.which() << '\n';
  INTERNAL_ERROR;
  return nullptr;
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
    return new Assign(newVar, getExpression(s, vd->val), s);
  }
  return nullptr;
}

Statement* addLocalVariable(BlockScope* s, string name, TypeSystem::Type* type, Expression* init)
{
  //Make sure variable doesn't already exist (shadowing var is error)
  Parser::Member search;
  search.ident = name;
  if(s->findVariable(&search))
  {
    ERR_MSG(string("variable \"") + name + "\" already exists");
  }
  Variable* newVar = new Variable(s, name, type);
  s->vars.push_back(newVar);
  if(init)
  {
    return new Assign(newVar, init, s);
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

For::For(Parser::For* f, Block* b)
{
  loopBlock = new Block(f, this, b);
  auto loopScope = f->body->bs;
  auto enclosing = loopScope->parent;
  Expression* one = new IntLiteral(1);
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
      condition = getExpression(enclosing, fc->condition);
      if(condition->type != TypeSystem::primitives[Parser::TypeNT::BOOL])
      {
        ERR_MSG("condition expression in C-style for loop must be a boolean");
      }
    }
    if(fc->incr)
    {
      increment = createStatement(loopBlock, fc->incr);
    }
  }
  else if(f->f.is<Parser::ForRange1*>() || f->f.is<Parser::ForRange2*>())
  {
    //introduce counter (counter type must be integer and is determined from the upper bound expression)
    //counter var names start at i and go to z, after that there is error
    Expression* lowerBound = nullptr;
    Expression* upperBound = nullptr;
    if(f->f.is<Parser::ForRange1*>())
    {
      auto f1 = f->f.get<Parser::ForRange1*>();
      //ForRange1 has a single expression, can be integer counter or array
      auto expr = getExpression(enclosing, f1->expr);
      if(expr->type && expr->type->isInteger())
      {
        lowerBound = new IntLiteral((uint64_t) 0);
        upperBound = expr;
      }
      else if(expr->type && expr->type->isArray())
      {
        errAndQuit("for loop over array isn't supported (yet)");
      }
      else
      {
        errAndQuit("invalid type as for loop range: must be integer or (not yet) array");
      }
    }
    else
    {
      auto f2 = f->f.get<Parser::ForRange2*>();
      lowerBound = getExpression(enclosing, f2->start);
      upperBound = getExpression(enclosing, f2->end);
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
      if(!enclosing->findVariable(&mem))
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
    finalCountMem.ident = counterName;
    Variable* counter = loopScope->findVariable(&finalCountMem);
    auto counterExpr = new VarExpr(loopScope, counter);
    //the condition is counterExpr < upperBound
    condition = new BinaryArith(counterExpr, CMPL, upperBound);
    //the increment is counter = counter + one
    increment = new Assign(counter, new BinaryArith(counterExpr, PLUS, one), loopScope);
  }
  loopBlock->addStatements();
}

While::While(Parser::While* w, Block* b)
{
  auto enclosing = b->scope;
  condition = getExpression(enclosing, w->cond);
  if(condition->type != TypeSystem::primitives[Parser::TypeNT::BOOL])
  {
    ERR_MSG("while loop condition must be a bool");
  }
  loopBlock = new Block(w, this, b);
}

If::If(Parser::If* i, Block* b)
{
  condition = getExpression(b->scope, i->cond);
  if(condition->type != TypeSystem::primitives[Parser::TypeNT::BOOL])
  {
    ERR_MSG("while loop condition must be a bool");
  }
  body = createStatement(b, i->ifBody);
}

IfElse::IfElse(Parser::If* i, Block* b)
{
  condition = getExpression(b->scope, i->cond);
  if(condition->type != TypeSystem::primitives[Parser::TypeNT::BOOL])
  {
    ERR_MSG("while loop condition must be a bool");
  }
  trueBody = createStatement(b, i->ifBody);
  falseBody = createStatement(b, i->elseBody);
}

Return::Return(Parser::Return* r, Block* b)
{
  from = b->subr;
  value = nullptr;
  bool voidReturn = r->ex == nullptr;
  if(!voidReturn)
  {
    value = getExpression(b->scope, r->ex);
  }
  //Make sure that the return expression has a type that matches the subroutine's retType
  if(voidReturn && b->subr->retType != TypeSystem::primitives[Parser::TypeNT::BOOL])
  {
    ERR_MSG("function or procedure doesn't return void, so return must have an expression");
  }
  if(!voidReturn && b->subr->retType == TypeSystem::primitives[Parser::TypeNT::BOOL])
  {
    ERR_MSG("procedure returns void but a return expression was provided");
  }
  if(!voidReturn && !b->subr->retType->canConvert(value))
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

Print::Print(Parser::PrintNT* p, BlockScope* s)
{
  for(auto e : p->exprs)
  {
    exprs.push_back(getExpression(s, e));
  }
}

Assertion::Assertion(Parser::Assertion* as, BlockScope* s)
{
  asserted = getExpression(s, as->expr);
}

Subroutine::Subroutine(Parser::SubroutineNT* snt, Scope* s)
{
  name = snt->name;
  scope = s;
  //first, compute the type by building a SubroutineTypeNT
  Parser::SubroutineTypeNT stypeNT;
  stypeNT.retType = snt->retType;
  stypeNT.params = snt->params;
  stypeNT.isPure = snt->isPure;
  stypeNT.nonterm = snt->nonterm;
  TypeNT tnt;
  tnt.t = &stypeNT;
  auto cnt = dynamic_cast<CallableType*>(TypeSystem::lookupType(&tnt, scope));
  if(!cnt)
  {
    ERR_MSG("subroutine " << name << " has an undefined type\n";
  }
  //link scope and this (both ways)
  snt->scope = 
  //process all statements
}

