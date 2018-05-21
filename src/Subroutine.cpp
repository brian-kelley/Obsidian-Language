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
  scope = new Scope(s->scope, this);
  statementCount = 0;
}

Block::Block(Block* parent)
{
  breakable = parent->breakable;
  loop = parent->loop;
  subr = parent->subr;
  scope = new Scope(parent->scope, this);
  statementCount = 0;
}

//Block which is a for loop body
//The loop body has the same index as the loop itself
Block::Block(For* f, Block* parent)
{
  subr = parent->subr;
  loop = f;
  breakable = f;
  scope = new Scope(parent->scope, this);
  statementCount = 0;
}

//Block which is a while loop body
Block::Block(While* w)
{
  Block* parent = w->body->scope->parent->node.get<Block*>();
  subr = parent->subr;
  loop = w;
  breakable = w;
  scope = new Scope(parent->scope, this);
  statementCount = 0;
}

Block::Block(Scope* s)
{
  subr = nullptr;
  loop = None();
  breakable = None();
  statementCount = 0;
  scope = new Scope(s, this);
}

void Block::addStatement(Statement* s)
{
  stmts.push_back(s);
  statementCount++;
}

void Block::resolveImpl(bool final)
{
  //Block needs to resolve both child statements and declarations in scope
  for(auto& stmt : stmts)
  {
    stmt->resolve(final);
    if(!stmt->resolved)
      return;
  }
  for(auto& decl : scope->names)
  {
    Node* n = decl.second.item;
    n->resolve(final);
    if(!n->resolved)
      return;
  }
  resolved = true;
}

Assign::Assign(Block* b, Expression* lhs, Expression* rhs) : Statement(b)
{
  lvalue = lhs;
  rvalue = rhs;
}

void Assign::resolveImpl(bool final)
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

CallStmt(Block* b, CallExpr* e) : Statement(b)
{
  eval = e;
}

void CallStmt::resolveImpl(bool final)
{
  eval->resolve(final);
  if(!eval->resolved)
    return;
  resolved = true;
}

For::For(Block* b) : Statement(b)
{
  outer = new Block(b);
  inner = new Block(outer);
  inner->loop = this;
  inner->breakable = this;
}

ForC::ForC(Block* b) : For(b)
{
  init = nullptr;
  condition = nullptr;
  increment = nullptr;
}

void ForC::resolveImpl(bool final)
{
  //Resolving outerwill only resolve the declarations in outer's scope,
  //since no statements will be added to outer
  outer->resolve(final);
  if(!outer->resolved)
    return;
  init->resolve(final);
  if(!init->resolved)
    return;
  condition->resolve(final);
  if(!condition->resolved)
    return;
  if(condition->type != primitives[Prim::BOOL])
  {
    errMsgLoc(condition, "C-style for loop condition must be a bool");
  }
  increment->resolve(final);
  if(!increment->resolved)
    return;
  //finally, resolve the body
  inner->resolve(final);
  if(!inner->resolved)
    return;
  resolved = true;
}

ForArray::ForArray(Block* b) : For(b)
{}

void ForArray::createIterators(vector<string>& iters)
{
  //parser should check for this anyway, but:
  if(iters.size() < 2)
  {
    INTERNAL_ERROR;
  }
  //create counters and iterator as variables in outer block
  for(size_t i = 0; i < iters.size() - 1; i++)
  {
    Variable* cnt = new Variable(iters[i], primitives[Prim::ULONG], outer);
    counters.push_back(cnt);
    outer->scope->addName(cnt);
  }
  //create iterator
  iter = new Variable(iters.back(), new ElemExprType(arr), outer);
}

void ForArray::resolveImpl(bool final)
{
  //just resolve outer, then inner
  outer->resolve(final);
  if(!outer->resolved)
    return;
  inner->resolve(final);
  if(!inner->resolved)
    return;
  resolved = true;
}

Block* ForArray::getInnerBody()
{
  Block* b = new Block(outerBody);
  outerBody->addStatement(b);
  b->loop = this;
  return b;
}

void ForArray::resolveImpl(bool final)
{
  resolveExpr(arr, final);
  if(!arr->resolved)
    return;
  //create the iteration variable since type of arr is known
  ArrayType* arrType = dynamic_cast<ArrayType*>(arr->type);
  if(!arrType)
  {
    errMsgLoc(this, "can't iterate over non-array expression");
  }
  if(counters.size() > arrType->dims)
  {
    errMsgLoc(this, "given " << counters.size() <<
        " loop counters but array only has " << arrType->dims << " dimensions");
  }
  if(!iter)
  {
    //find the element type
    auto iterType = TypeSystem::getArrayType(arrType->elem, arrType->dims - counters.size());
    iter = new Variable(outerBody->scope, iterName, iterType, false);
    outerBody->scope->addName(iter);
  }
  outerBody->resolve(final);
  if(outerBody->resolved)
    resolved = true;
}

While::While(Block* b, Expression* cond, Block* whileBody)
  : Statement(b)
{
  condition = cond;
  body = whileBody;
}

void While::resolveImpl(bool final)
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

If::If(Block* b, Expression* cond, Statement* b)
  : Statement(b)
{
  condition = cond;
  body = b;
}

If::If(Block* b, Expression* cond, Statement* tb, Statement* fb)
  : Statement(b)
{
  condition = cond;
  body = tb;
  elseBody = fb;
}

void If::resolveImpl(bool final)
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

Match::Match(Block* b, Expression* m, string varName,
    vector<TypeSystem::Type*>& t,
    vector<Block*>& caseBlocks)
  : Statement(b)
{
  matched = m;
  types = t;
  blocks = caseBlocks;
  //create blocks to enclose each case block, and
  //add the value variables to each
  if(types.size() != blocks.size())
  {
    INTERNAL_ERROR;
  }
  int n = types.size();
  caseVars.resize(n);
  for(int i = 0; i < n; i++)
  {
    Block* outerBlock = new Block(b);
    outerBlock->setLocation(blocks[i]);
    caseVars[i] = new Variable(varName, types[i], outerBlock);
    outerBlock->scope->addName(caseVars[i]);
    outerBlock->stmts.push_back(outerBlock);
    Block* innerBlock = blocks[i];
    innerBlock->scope->node = outerBlock;
    blocks[i] = outerBlock;
  }
}

void Match::resolveImpl(bool final)
{
  resolveExpr(matched, final);
  if(!matched->resolved)
    return;
  auto ut = dynamic_cast<UnionType*>(matched->type);
  if(!ut)
  {
    errMsgLoc(matched, "matched expression must be of union type");
  }
  for(auto& t : types)
  {
    resolveType(t, final);
    if(!t->resolved)
      return;
  }
  for(auto t : types)
  {
    if(find(ut->options.begin(), ut->options.end(), t) == ut->options.end())
    {
      errMsgLoc(this, "match includes type " << t->getName() << " which is not a member of union " << ut->getName());
    }
  }
  bool allResolved = true;
  for(auto b : blocks)
  {
    b->resolve(final);
    if(!b->resolved)
      allResolved = false;
  }
  if(allResolved)
    resolved = true;
}

Switch::Switch(Block* b, Expression* s, vector<int>& inds, vector<Expression*> vals, vector<Statement*>& stmtList, int defaultPos)
  : Statement(b)
{
  switched = s;
  caseValues = vals;
  caseLabels = inds;
  defaultPosition = defaultPos;
  stmts = stmtList;
}

void Switch::resolveImpl(bool final)
{
  resolveExpr(switched, final);
  if(!switched->resolve)
    return;
  //resolve case values and make sure they can convert to 
  bool allResolved = true;
  for(auto& caseVal : caseValues)
  {
    resolveExpr(caseVal, final);
    if(!caseVal->resolved)
    {
      allResolved = false;
    }
    else
    {
      if(!switched->type->canConvert(caseVal->type))
      {
        errMsgLoc(caseVal, "case value type incompatible with switch value type");
      }
      else if(switched->type != caseVal->type)
      {
        caseVal = new Converted(caseVal, switched->type);
      }
    }
  }
  //resolve all the statements
  for(auto& stmt : stmts)
  {
    stmt->resolve(final);
    if(!stmt->resolved)
      allResolved = false;
  }
  if(allResolved)
    resolved = true;
}

Return::Return(Block* b, Expression* e) : Statement(b)
{
  value = e;
}

Return(Block* b) : Statement(b)
{
  value = nullptr;
}

void Return::resolveImpl(bool final)
{
  if(value)
  {
    value->resolve(final);
    if(!value->resolved)
      return;
  }
  //make sure value can be converted to enclosing subroutine's return type
  auto subrRetType = block->subr->type->retType;
  if(subrRetType == primitives[Prim::VOID])
  {
    if(value)
    {
      errMsgLoc(this, "returned a value from void subroutine");
    }
  }
  if(!subrRetType->canConvert(value->type))
  {
    errMsgLoc(this, "returned value of type " << value->type->getName() << " incompatible with subroutine return type " << subrRetType->getName());
  }
  else if(subrRetType != value->type)
  {
    value = new Converted(value, subrRetType);
  }
  resolved = true;
}

Break::Break(Block* b) : Statement(b)
{}

void Break::resolveImpl(bool final)
{
  if(block->breakable.is<None>())
  {
    errMsgLoc(this, "break is not inside any loop or switch");
  }
  breakable = block->breakable;
}

Continue::Continue(Block* b) : Statement(b)
{}

void Continue::resolveImpl(bool final)
{
  if(block->loop.is<None>())
  {
    errMsgLoc(this, "continue is not inside any loop");
  }
  loop = block->loop;
}

Print::Print(Block* b, vector<Expression*>& e) : Statement(b)
{
  exprs = e;
}

void Print::resolveImpl(bool final)
{
  for(auto& e : exprs)
  {
    resolveExpr(e, final);
    if(!e->resolved)
    {
      return;
    }
  }
  resolved = true;
}

Assertion::Assertion(Block* b, Expression* a) : Statement(b)
{
  asserted = a;
}

void Assertion::resolveImpl(bool final)
{
  resolveExpr(asserted, final);
  if(!asserted->resolved)
  {
    return;
  }
  if(asserted->type != primitives[Prim::BOOL])
  {
    errMsgLoc("asserted value has non-bool type " << asserted->type->getName());
  }
  resolved = true;
}

Subroutine::Subroutine(Scope* s, string name, bool isStatic, bool pure, TypeSystem::Type* returnType, vector<string>& argNames, vector<TypeSystem::Type*>& argTypes, Block* body)
{
  name = n;
  scope = new Scope(enclosing, this);
  auto enclosingStruct = scope->getMemberContext();
  if(enclosingStruct && !isStatic)
  {
    type = new CallableType(pure, enclosingStruct, returnType, argTypes);
    owner = enclosingStruct;
  }
  else
  {
    type = new CallableType(pure, returnType, argTypes);
  }
  //create argument variables
  if(argNames.size() != argTypes.size())
  {
    INTERNAL_ERROR;
  }
  for(size_t i = 0; i < argNames.size(); i++)
  {
    Variable* v = new Variable(scope, argNames[i], argTypes[i], true);
    args.push_back(v);
    scope->addName(v);
  }
  body = bodyBlock;
}

void Subroutine::resolveImpl(bool final)
{
  type->resolve(final);
  if(!type->resolved)
    return;
  for(auto arg : args)
  {
    //resolving argument variables just resolves their types
    arg->resolve(final);
    if(!arg->resolved)
    {
      return;
    }
  }
  //resolve the body
  body->resolve(final);
  if(!body->resolved)
    return;
  resolved = true;
}

ExternalSubroutine::ExternalSubroutine(Scope* s, string name, TypeSystem::Type* returnType, vector<TypeSystem::Type*>& argTypes, vector<string>& argN, string& code)
{
  type = new CallableType(false, returnType, argTypes);
  c = code;
  argNames = argN;
}

void ExternalSubroutine::resolveImpl(bool final)
{
  type->resolve(final);
  if(!type->resolved)
    return;
  resolved = true;
}

Test::Test(Scope* s, Block* b) : scope(s), run(b)
{
  tests.push_back(this);
}

void Test::resolveImpl(bool final)
{
  run->resolve(final);
  resolved = run->resolved;
}

