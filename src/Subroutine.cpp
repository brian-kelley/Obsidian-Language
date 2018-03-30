#include "Subroutine.hpp"
#include "Variable.hpp"

using namespace TypeSystem;

extern bool programHasMain;
extern ModuleScope* global;

vector<Test*> Test::tests;

//Block which is body of subroutine
Block::Block(Subroutine* s)
{
  breakable = None;
  loop = None;
  subr = s;
  scope = new Scope(s->scope);
  scope->node = this;
  statementCount = 0;
}

Block::Block(Block* parent)
{
  breakable = parent->breakable;
  loop = parent->loop;
  subr = parent->subr;
  scope = new Scope(parent->scope);
  scope->node = this;
  statementCount = 0;
}

//Block which is a for loop body
//The loop body has the same index as the loop itself
Block::Block(For* f, Block* parent)
{
  subr = parent->subr;
  loop = f;
  breakable = f;
  scope = new Scope(parent->scope);
  scope->node = this;
  statementCount = 0;
}

//Block which is a while loop body
Block::Block(While* w)
{
  Block* parent = w->body->scope->parent->node.get<Block*>();
  subr = parent->subr;
  loop = w;
  breakable = w;
  scope = new Scope(parent->scope);
  scope->node = this;
  statementCount = 0;
}

Assign::Assign(Expression* lhs, Expression* rhs)
{
  lvalue = lhs;
  rvalue = rhs;
}

void Assign::resolve(bool final)
{
  resolveExpr(lvalue, final);
  resolveExpr(rvalue, final);
  if(!lvalue->resolved || !rvalue->resolved)
  {
    return;
  }
  if(!lvalue->assignable())
  {
    errMsgLoc(this, "left-hand side of assignment is immutable");
  }
  if(!rvalue->type->canConvert(lvalue->type))
  {
    errMsgLoc(this, "cannot convert from " << rvalue->type->getName() << " to " << lvalue->type->getName());
  }
  if(lvalue->type != rvalue->type)
  {
    rvalue = new Converted(rvalue, lvalue->type);
    rvalue->resolve(true);
  }
  resolved = true;
}

CallStmt(CallExpr* e)
{
  eval = e;
}

void CallStmt::resolve(bool final)
{
  eval->resolve(final);
  if(!eval->resolved)
    return;
  resolved = true;
}

For::For(Statement* i, Expression* cond, Statement* incr, Block* b)
{
  //variables declared in initialization should be in body's scope
  init = i;
  condition = cond;
  increment = incr;
  body = b;
}

For::For(vector<string>& tupIter, Expression* arr, Block* innerBody)
{
  //recursively create the syntax of a nested for loop over each dim
  //body is the bodyh of innermost loop only
  if(tupIter.size() < 2)
  {
    errMsgLoc(this, "for over array needs >= 1 counter and an iteration variable");
  }
  body = new Block(this);
  setupRange(tupIter.front(), new IntLiteral(0), new ArrayLength(arr));
  //get arr[i] where i is this dimension's counter
  Expression* subArr = new Indexed(arr,
      new UnresolvedExpr(tupIter.front(), body->scope));
  if(tupIter.size() == 2)
  {
    //innermost loop
    //create the final iteration variable and assign subArr to it
    Variable* iterVar = new Variable(tupIter.back(), new ExprType(subArr), Block* b);
    body->scope->addName(iterVar);
    body->stmts.push_back(new Assign(new VarExpr(iterVar), subArr));
    //finally, add the inner body,
    //and fix its position in scope tree
    innerBody->scope->parent = body->scope;
    body->stmts.push_back(innerBody);
  }
  else
  {
    //not innermost loop,
    //only statement in body is another For (with same innerBody)
    vector<string> nextTupIter(tupIter.size() - 1);
    for(size_t i = 1; i < tupIter.size(); i++)
    {
      nextTupIter[i - 1] = tupIter[i];
    }
    body->stmts.push_back(new For(nextTupIter, subArr, innerBody));
  }
}

For::For(string counter, Expression* begin, Expression* end, Block* b)
{
  body = new Block(this);
  setupRange(counter, begin, end);
  body->stmts.push_back(b);
  b->scope->parent = body->scope;
}

void For::resolve(bool final)
{
  init->resolve(final);
  if(!init->resolved)
    return;
  resolveExpr(condition, final);
  if(!condition->resolved)
    return;
  increment->resolve(final);
  if(!increment->resolved)
    return;
  body->resolve(final);
  if(!body->resolved)
    return;
  resolved = true;
}

Variable* For::setupRange(string counterName, Expression* begin, Expression* end)
{
  //get the correct type for the counter
  Variable* counterVar = new Variable(counterName, primitives[Prim::LONG], body);
  body->scope->addName(counterVar);
  Expression* counterExpr = new VarExpr(counterVar);
  counterExpr->resolve(true);
  init = new VarAssign(counterExpr, new IntLiteral(0));
  condition = new BinaryArith(counterExpr, CMPL, end);
  increment = new VarAssign(counterExpr, new BinaryArith(counterExpr, PLUS, new IntLiteral(1)));
  return counterVar;
}

While::While(Expression* cond, Block* b)
{
  condition = cond;
  body = b;
}

void While::resolve(bool final)
{
  resolveExpr(condition, final);
  if(!condition->resolved)
    return;
  if(condition->type != primitives[Prim::BOOL])
  {
    errMsgLoc(condition, "while loop condition must be bool");
  }
  body->resolve(final);
  if(!body->resolved)
    return;
  resolved = true;
}

If::If(Expression* cond, Statement* b)
{
  condition = cond;
  body = b;
}

If::If(Expression* cond, Statement* tb, Statement* fb)
{
  condition = cond;
  body = tb;
  elseBody = fb;
}

void If::resolve(bool final)
{
  resolveExpr(condition, final);
  if(!condition->resolved)
    return;
  body->resolve(final);
  if(!body->resolved)
    return;
  if(elseBody)
  {
    elseBody->resolve(final);
    if(!elseBody->resolved)
      return;
  }
  resolved = true;
}

Match::Match(Parser::Match* m, Block* b)
{
  matched = getExpression(b->scope, m->value);
  //get the relevant union type
  UnionType* ut = dynamic_cast<UnionType*>(matched->type);
  if(!ut)
  {
    ERR_MSG("match statement given a non-union expression");
  }
  //check for # of cases mismatch
  if(ut->options.size() != m->cases.size())
  {
    ERR_MSG("number of match cases differs from number of union type options");
  }
  cases = vector<Block*>(ut->options.size(), nullptr);
  caseVars = vector<Variable*>(ut->options.size(), nullptr);
  //for each parsed case, get the type and find correct slot in cases
  for(auto c : m->cases)
  {
    Type* caseType = lookupType(c.type, b->scope);
    if(!caseType)
    {
      ERR_MSG("unknown type as match case");
    }
    int i = 0;
    for(auto option : ut->options)
    {
      if(caseType == option)
        break;
      i++;
    }
    if(i == ut->options.size())
    {
      ERR_MSG("given match case type is not in union");
    }
    if(cases[i])
    {
      ERR_MSG("match case has same type as a previous case");
    }
    auto caseBlock = c.block;
    //create the block as a child of b, but it doesn't get run unconditionally
    cases[i] = new Block(blockScopes[caseBlock], b);
    //create and add the value variable, which will be initialized in code gen
    caseVars[i] = new Variable(cases[i]->scope, m->varName, caseType);
    cases[i]->scope->addName(caseVars[i]);
    //add statements to block
    cases[i]->addStatements(caseBlock);
  }
}

Switch::Switch(Parser::Switch* s, Block* b)
{
  switched = getExpression(b->scope, s->value);
  for(auto& label : s->labels)
  {
    Expression* caseExpr = getExpression(b->scope, label.value);
    //make sure the case value can be converted to switched->type
    if(!caseExpr->type->canConvert(switched->type))
    {
      ERR_MSG("switched case value can't be compared with switched expression");
    }
    if(caseExpr->type != switched->type)
    {
      caseExpr = new Converted(caseExpr, switched->type);
    }
    caseValues.push_back(caseExpr);
    caseLabels.push_back(label.position);
  }
  defaultPosition = s->defaultPosition;
  //create the block and add statements
  block = new Block(blockScopes[s->block], b);
  block->breakable = this;
  //add all the statements right away
  block->addStatements(s->block);
}

Return::Return(Parser::Return* r, Block* b)
{
  from = b->subr;
  value = nullptr;
  if(r->ex)
  {
    value = getExpression(b->scope, r->ex);
  }
  Type* voidType = primitives[Parser::TypeNT::VOID];
  Type* subrRetType = b->subr->type->returnType;
  Type* actualRetType = voidType;
  if(value)
    actualRetType = value->type;
  //Make sure that the return expression has a type that matches the subroutine's retType
  if(subrRetType != voidType && actualRetType == voidType)
  {
    ERR_MSG("subroutine returns non-void but return not given expression");
  }
  if(subrRetType == voidType && actualRetType != voidType)
  {
    ERR_MSG("subroutine returns void but a return expression was provided");
  }
  //see if value conversion necessary
  if(subrRetType != actualRetType)
  {
    value = new Converted(value, subrRetType);
  }
}

Return::Return(Subroutine* s)
{
  value = nullptr; //void return
  from = s;
  //no other checking necessary
}

Break::Break(Block* b)
{
  //make sure the break is inside a loop
  if(b->breakable.is<None>())
  {
    ERR_MSG("break statement used outside of a for, while or switch");
  }
  breakable = b->breakable;
}

Continue::Continue(Block* b)
{
  //make sure the continue is inside a loop
  if(b->loop.is<None>())
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
  scope = (SubroutineScope*) s;
  body = nullptr;
  auto stypeNT = new Parser::SubroutineTypeNT;
  stypeNT->retType = snt->retType;
  stypeNT->params = snt->params;
  stypeNT->isStatic = snt->isStatic;
  stypeNT->isPure = snt->isPure;
  stypeNT->nonterm = snt->nonterm;
  TypeLookup tl(stypeNT, scope);
  TypeSystem::typeLookup->lookup(tl, (Type*&) type);
}

void Subroutine::check()
{
  //Need special checks for main
  //ret type can be void or int
  //args are either string[] or nothing
  auto voidType = primitives[Parser::TypeNT::VOID];
  if(name == "main")
  {
    if(type->pure)
    {
      ERR_MSG("main() must be a procedure");
    }
    if(scope->parent != global)
    {
      ERR_MSG("main() is not in global scope");
    }
    programHasMain = true;
    if(type->returnType != voidType &&
        type->returnType != primitives[Parser::TypeNT::INT])
    {
      ERR_MSG("proc main must return void or int");
    }
    bool noArgs = type->argTypes.size() == 0;
    bool takesStringArray = type->argTypes.size() == 1 &&
      type->argTypes[0] == getArrayType(primitives[Parser::TypeNT::CHAR], 2);
    if(!noArgs && !takesStringArray)
    {
      ERR_MSG("proc main must take no arguments or only an array of strings");
    }
  }
  body->check();
  //after checking body, check if it ends in a return
  //if return type is void and there is no return, add it explicitly
  //TODO: check for "missing return" when CFG is supported
  if(type->returnType == voidType &&
      (body->stmts.size() == 0 || !dynamic_cast<Return&*>(body->stmts.back())))
  {
    body->stmts.push_back(new Return(this));
  }
}

ExternalSubroutine::ExternalSubroutine(Parser::ExternSubroutineNT* es, Scope* s)
{
  TypeSystem::typeLookup->lookup(es->type, (Type*&) type);
  c = es->c;
}

Test::Test(Parser::TestDecl* td, Scope* s)
{
  tests.push_back(this);
  //Create a dummy block
  //to hold the statement
  BlockScope* bs = blockScopes[td->block];
  run = new Block(bs);
  run->addStatements(td->block);
}

