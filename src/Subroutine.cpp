#include "Subroutine.hpp"
#include "Variable.hpp"

bool programHasMain = false;
extern Module* global;

vector<Test*> Test::tests;

//Block which is body of subroutine
Block::Block(Subroutine* s)
{
  breakable = None();
  loop = None();
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

Assign::Assign(Block* b, Expression* lhs, int op, Expression* rhs)
  : Statement(b)
{
  //the actual rvalue used internally depends on the operation
  lvalue = lhs;
  switch(op)
  {
    case ASSIGN:
      rvalue = rhs;
      break;
    case PLUSEQ:
    case SUBEQ:
    case MULEQ:
    case DIVEQ:
    case MODEQ:
    case BOREQ:
    case BANDEQ:
    case BXOREQ:
    case SHLEQ:
    case SHREQ:
      rvalue = new BinaryArith(lhs, op, rhs);
      break;
    case INC:
      rvalue = new BinaryArith(lhs, PLUS, new IntLiteral(1));
      break;
    case DEC:
      rvalue = new BinaryArith(lhs, SUB, new IntLiteral(1));
      break;
    default:
      errMsgLoc(this, "invalid operation for assignment");
  }
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

CallStmt::CallStmt(Block* b, CallExpr* e) : Statement(b)
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
    Variable* cnt = new Variable(iters[i], primitives[Prim::LONG], outer);
    counters.push_back(cnt);
    outer->scope->addName(cnt);
  }
  //create iterator
  iter = new Variable(iters.back(), new ElemExprType(arr), outer);
}

void ForArray::resolveImpl(bool final)
{
  resolveExpr(arr, final);
  if(!arr->resolved)
    return;
  ArrayType* arrType = dynamic_cast<ArrayType*>(arr->type);
  if(!arrType)
  {
    errMsgLoc(this, "can't iterate over non-array expression");
  }
  if(arrType->dims < counters.size())
  {
    errMsgLoc(this, "requested " << counters.size() <<
        " counters but array has only " << arrType->dims << " dimensions");
  }
  //finally resolve outer and inner blocks
  //resolving outer will also resolve the counters and iter
  outer->resolve(final);
  if(!outer->resolved)
    return;
  inner->resolve(final);
  if(!inner->resolved)
    return;
  resolved = true;
}

ForRange::ForRange(Block* b, string counterName, Expression* beginExpr, Expression* endExpr)
  : For(b), begin(beginExpr), end(endExpr)
{
  //create the counter variable in outer block
  counter = new Variable(counterName, primitives[Prim::LONG], outer);
}

void ForRange::resolveImpl(bool final)
{
  resolveExpr(begin, final);
  if(!begin->resolved)
    return;
  resolveExpr(end, final);
  if(!end->resolved)
    return;
  outer->resolve(final);
  if(!outer->resolved)
    return;
  inner->resolve(final);
  if(!inner->resolved)
    return;
  resolved = true;
}

While::While(Block* b, Expression* cond)
  : Statement(b)
{
  condition = cond;
  body = new Block(b);
  body->loop = this;
  body->breakable = this;
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

If::If(Block* b, Expression* cond, Statement* bodyStmt)
  : Statement(b)
{
  condition = cond;
  body = bodyStmt;
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
    vector<Type*>& t,
    vector<Block*>& caseBlocks)
  : Statement(b)
{
  matched = m;
  types = t;
  cases = caseBlocks;
  //create blocks to enclose each case block, and
  //add the value variables to each
  if(types.size() != cases.size())
  {
    INTERNAL_ERROR;
  }
  int n = types.size();
  caseVars.resize(n);
  for(int i = 0; i < n; i++)
  {
    caseVars[i] = new Variable(varName, types[i], cases[i]);
    cases[i]->scope->addName(caseVars[i]);
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
  for(auto b : cases)
  {
    b->resolve(final);
    if(!b->resolved)
      allResolved = false;
  }
  if(allResolved)
    resolved = true;
}

Switch::Switch(Block* b, Expression* s, vector<int>& inds, vector<Expression*> vals, int defaultPos, Block* stmtBlock)
  : Statement(b)
{
  switched = s;
  caseValues = vals;
  caseLabels = inds;
  defaultPosition = defaultPos;
  block = stmtBlock;
}

void Switch::resolveImpl(bool final)
{
  resolveExpr(switched, final);
  if(!switched->resolved)
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
  //this resolves all statements
  block->resolve(final);
  if(!block->resolved)
    return;
  resolved = true;
}

Return::Return(Block* b, Expression* e) : Statement(b)
{
  value = e;
}

Return::Return(Block* b) : Statement(b)
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
  auto subrRetType = block->subr->type->returnType;
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
    errMsgLoc(this, "asserted value has non-bool type " << asserted->type->getName());
  }
  resolved = true;
}

Subroutine::Subroutine(Scope* enclosing, string n, bool isStatic, bool pure, Type* returnType, vector<string>& argNames, vector<Type*>& argTypes)
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
    Variable* v = new Variable(scope, argNames[i], argTypes[i], nullptr, true);
    args.push_back(v);
    scope->addName(v);
  }
  body = new Block(this);
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
  if(name == "main")
  {
    if(scope->parent != global->scope)
    {
      errMsgLoc(this, "main must be in global scope");
    }
    if(type->pure)
    {
      errMsgLoc(this, "main must be a procedure, not a function");
    }
    if(type->returnType != primitives[Prim::VOID] &&
        type->returnType != primitives[Prim::INT])
    {
      errMsgLoc(this, "main() must return void or int");
    }
    if(type->argTypes.size() > 1 ||
        (type->argTypes.size() == 1 &&
         type->argTypes[0] != getArrayType(primitives[Prim::CHAR], 2)))
    {
      errMsgLoc(this, "main() must take either no arguments or string[]");
    }
    programHasMain = true;
  }
  resolved = true;
}

ExternalSubroutine::ExternalSubroutine(Scope* s, string n, Type* returnType, vector<Type*>& argTypes, vector<string>& argN, string& code)
{
  //all ExternalSubroutines are procedures, since it is assumed that
  //all C functions may have side effects
  type = new CallableType(false, returnType, argTypes);
  name = n;
  c = code;
  scope = s;
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

