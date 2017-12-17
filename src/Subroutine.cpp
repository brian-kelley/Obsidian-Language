#include "Subroutine.hpp"
#include "Variable.hpp"

extern map<Parser::Block*, BlockScope*> blockScopes;

//Block which is body of subroutine
Block::Block(Parser::Block* b, BlockScope* s, Subroutine* sub) : scope(s)
{
  this->subr = sub;
  this->loop = nullptr;
  addStatements(b);
}

//Block which is child of another block
Block::Block(Parser::Block* b, BlockScope* s, Block* parent) : scope(s)
{
  this->subr = parent->subr;
  this->loop = parent->loop;
  addStatements(b);
}

Block::Block(BlockScope* s, Block* parent)
{
  this->subr = parent->subr;
  this->loop = parent->loop;
}

//Block which is a for loop body
Block::Block(Parser::For* forAST, For* f, BlockScope* s, Block* parent) : scope(s)
{
  this->subr = parent->subr;
  this->loop = new Loop(f);
  addStatements(forAST->body);
}
//Block which is a while loop body
Block::Block(Parser::While* whileAST, While* w, BlockScope* s, Block* parent) : scope(s)
{
  this->subr = parent->subr;
  this->loop = new Loop(w);
  addStatements(whileAST->body);
}

void Block::addStatements(Parser::Block* ast)
{
  for(auto stmt : ast->statements)
  {
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
  else if(stmt->s.is<Parser::Expr12*>())
  {
    return new CallStmt(stmt->s.get<Parser::Expr12*>(), scope);
  }
  else if(stmt->s.is<Parser::Block*>())
  {
    auto block = stmt->s.get<Parser::Block*>();
    return new Block(block, blockScopes[block], b);
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
    return nullptr;
  }
  cout << "About to internal error, actual Parser Statement tag: " << stmt->s.which() << '\n';
  INTERNAL_ERROR;
  return nullptr;
}

Statement* addLocalVariable(BlockScope* s, Parser::VarDecl* vd)
{
  //Create variable
  Variable* newVar = new Variable(s, vd);
  if(!newVar->type)
  {
    //all types must be available now
    //need to check here because variable ctor uses deferred type lookup
    ERR_MSG("variable " << newVar->name << " has unknown type");
  }
  //addName will check for shadowing
  s->addName(newVar);
  if(vd->val)
  {
    //add the initialization as a statement
    return new Assign(newVar, getExpression(s, vd->val), s);
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
  lvalue = new VarExpr(target);
  //vars are always lvalues, no need to check that
  if(!lvalue->type->canConvert(e))
  {
    ERR_MSG("incompatible types for assignment");
  }
  rvalue = e;
}

CallStmt::CallStmt(Parser::Expr12* call, BlockScope* s)
{
  eval = (CallExpr*) getExpression(s, call);
}

For::For(Parser::For* f, Block* b)
{
  BlockScope* loopScope = blockScopes[f->body];
  loopBlock = new Block(f, this, loopScope, b);
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
        ERR_MSG("condition in C-style for loop must be a boolean expression");
      }
    }
    if(fc->incr)
    {
      increment = createStatement(loopBlock, fc->incr);
    }
    loopBlock->addStatements();
  }
  else if(f->f.is<Parser::ForOverArray*>())
  {
    auto foa = f->f.get<Parser::ForOverArray*>();
    //get the array expression
    Expression* arr = getExpression(enclosing, foa->expr);
    //make sure arr is actually an array
    ArrayType* arrType = dynamic_cast<ArrayType*>(arr->type);
    if(!arrType)
    {
      ERR_MSG("for over array given non-array expression");
    }
    if(arrType->dims != foa->tup.size() - 1)
    {
      ERR_MSG("for over array iterating tuple has wrong size for given array");
    }
    //generate one for loop (including this one) as the body for each dimension
    //and the "it" value in the innermost loop
    For* dimLoop = this;
    Block* dimBlock = new loopBlock;
    for(int i = 0; i < arrType->dims; i++)
    {
      if(i > 0)
      {
        //construct next loop's scope as child of loopScope
        BlockScope* nextScope = new BlockScope(loopScope);
        For* nextFor = new For;
        nextFor->
      }
    }
  }
  else if(f->f.is<Parser::ForRange*>())
  {
    auto fr = f->f.get<Parser::ForRange*>();
    //Get start and end as expressions (their scopes are loop's parent)
    Expression* start = getExpression(loopScope->parent, fr->start);
    if(!start->type || !start->type->isInteger())
    {
      ERR_MSG("for over range: start value is not an integer");
    }
    Expression* end = getExpression(loopScope->parent, fr->end);
    if(!end->type || !end->type->isInteger())
    {
      ERR_MSG("for over range: end value is not an integer");
    }
    //get counter type: whatever type is compatible with both start and end
    Type* counterType = TypeSystem::promote(start->type, end->type);
    Variable* counter = new Variable(loopScope, foa->name, counterType);
    loopScope->addName(counter);
    init = new Assign(counter, start);
    condition = new BinaryArith(counter, CMPL, end);
    increment = new Assign(counter, new BinaryArith(counter, PLUS, one));
  }
  else
  {
    INTERNAL_ERROR;
  }
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
  nt = snt;
  name = nt->name;
  scope = s;
  body = nullptr;
  //first, compute the type by building a SubroutineTypeNT
  Parser::SubroutineTypeNT stypeNT;
  stypeNT.retType = snt->retType;
  stypeNT.params = snt->params;
  stypeNT.isPure = snt->isPure;
  stypeNT.nonterm = snt->nonterm;
  TypeLookup tl(stypeNT, s);
  type = (CallableType*) TypeSystem::typeLookup->lookup(tl);

}

void Subroutine::addStatements()
{
  body = new Block(nt->body, this);
}

