#include "Subroutine.hpp"
#include "Variable.hpp"
#include <algorithm>

using std::find;

static int nextSubrID = 0;
static int nextExSubrID = 0;

vector<Subroutine*> allSubrs;
vector<ExternalSubroutine*> allExSubrs;

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
}

Block::Block(Block* parent)
{
  breakable = parent->breakable;
  loop = parent->loop;
  subr = parent->subr;
  scope = new Scope(parent->scope, this);
}

Block::Block(Scope* s)
{
  subr = nullptr;
  loop = None();
  breakable = None();
  scope = new Scope(s, this);
}

void Block::addStatement(Statement* s)
{
  stmts.push_back(s);
}

void Block::resolveImpl()
{
  //Block needs to resolve both child statements and declarations in scope
  for(auto& stmt : stmts)
  {
    stmt->resolve();
  }
  for(auto& decl : scope->names)
  {
    Node* n = decl.second.item;
    n->resolve();
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
      {
        int nonAssignOp = -1;
        switch(op)
        {
          case PLUSEQ:
            nonAssignOp = PLUS; break;
          case SUBEQ:
            nonAssignOp = SUB; break;
          case MULEQ:
            nonAssignOp = MUL; break;
          case DIVEQ:
            nonAssignOp = DIV; break;
          case MODEQ:
            nonAssignOp = MOD; break;
          case BOREQ:
            nonAssignOp = BOR; break;
          case BANDEQ:
            nonAssignOp = BAND; break;
          case BXOREQ:
            nonAssignOp = BXOR; break;
          case SHLEQ:
            nonAssignOp = SHL; break;
          case SHREQ:
            nonAssignOp = SHR; break;
          default:;
        }
        rvalue = new BinaryArith(lhs, nonAssignOp, rhs);
        break;
      }
    case INC:
      {
        IntConstant* one = new IntConstant((int64_t) 1);
        one->type = primitives[Prim::BYTE];
        rvalue = new BinaryArith(lhs, PLUS, one);
        break;
      }
    case DEC:
      {
        IntConstant* one = new IntConstant((int64_t) 1);
        one->type = primitives[Prim::BYTE];
        rvalue = new BinaryArith(lhs, SUB, one);
        break;
      }
    default:
      errMsgLoc(this, "invalid operation for assignment");
  }
}

void Assign::resolveImpl()
{
  resolveExpr(lvalue);
  //Default-initialized local variables produce an assignment
  //with a null rvalue - use the default value for the type
  if(rvalue)
  {
    resolveExpr(rvalue);
  }
  else
  {
    rvalue = lvalue->type->getDefaultValue();
    rvalue->resolve();
  }
  if(!lvalue->assignable())
  {
    errMsgLoc(this, "left-hand side of assignment is immutable");
  }
  if(!lvalue->type->canConvert(rvalue->type))
  {
    errMsgLoc(this, "cannot convert from " << rvalue->type->getName() << " to " << lvalue->type->getName());
  }
  if(lvalue->type != rvalue->type)
  {
    rvalue = new Converted(rvalue, lvalue->type);
    rvalue->resolve();
  }
  resolved = true;
}

CallStmt::CallStmt(Block* b, CallExpr* e) : Statement(b)
{
  eval = e;
}

void CallStmt::resolveImpl()
{
  eval->resolve();
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

void ForC::resolveImpl()
{
  //Resolving outerwill only resolve the declarations in outer's scope,
  //since no statements will be added to outer
  outer->resolve();
  init->resolve();
  condition->resolve();
  if(condition->type != primitives[Prim::BOOL])
  {
    errMsgLoc(condition, "C-style for loop condition must be a bool");
  }
  increment->resolve();
  //finally, resolve the body
  inner->resolve();
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
  outer->scope->addName(iter);
}

void ForArray::resolveImpl()
{
  resolveExpr(arr);
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
  outer->resolve();
  inner->resolve();
  resolved = true;
}

ForRange::ForRange(Block* b, string counterName, Expression* beginExpr, Expression* endExpr)
  : For(b), begin(beginExpr), end(endExpr)
{
  //create the counter variable in outer block
  counter = new Variable(counterName, primitives[Prim::LONG], outer);
  outer->scope->addName(counter);
}

void ForRange::resolveImpl()
{
  resolveExpr(begin);
  resolveExpr(end);
  outer->resolve();
  inner->resolve();
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

void While::resolveImpl()
{
  resolveExpr(condition);
  if(condition->type != primitives[Prim::BOOL])
  {
    errMsgLoc(condition, "while loop condition must be bool");
  }
  body->resolve();
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

void If::resolveImpl()
{
  resolveExpr(condition);
  body->resolve();
  if(elseBody)
  {
    elseBody->resolve();
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

void Match::resolveImpl()
{
  resolveExpr(matched);
  auto ut = dynamic_cast<UnionType*>(matched->type);
  if(!ut)
  {
    errMsgLoc(matched, "matched expression must be of union type");
  }
  for(auto& t : types)
  {
    resolveType(t);
  }
  for(auto t : types)
  {
    if(find(ut->options.begin(), ut->options.end(), t) == ut->options.end())
    {
      errMsgLoc(this, "match includes type " << t->getName() << " which is not a member of union " << ut->getName());
    }
  }
  for(auto b : cases)
  {
    b->resolve();
  }
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

void Switch::resolveImpl()
{
  resolveExpr(switched);
  //resolve case values and make sure they can convert to 
  for(auto& caseVal : caseValues)
  {
    resolveExpr(caseVal);
    if(!switched->type->canConvert(caseVal->type))
    {
      errMsgLoc(caseVal, "case value type incompatible with switch value type");
    }
    else if(switched->type != caseVal->type)
    {
      caseVal = new Converted(caseVal, switched->type);
    }
  }
  //this resolves all statements
  block->resolve();
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

void Return::resolveImpl()
{
  if(value)
  {
    value->resolve();
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
  else if(!subrRetType->canConvert(value->type))
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

void Break::resolveImpl()
{
  if(block->breakable.is<None>())
  {
    errMsgLoc(this, "break is not inside any loop or switch");
  }
  breakable = block->breakable;
}

Continue::Continue(Block* b) : Statement(b)
{}

void Continue::resolveImpl()
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
  usage = b->scope;
}

void Print::resolveImpl()
{
  if(usage->getFunctionContext())
  {
    errMsgLoc(this, "print can't be used in a function");
  }
  for(auto& e : exprs)
  {
    resolveExpr(e);
  }
  resolved = true;
}

Assertion::Assertion(Block* b, Expression* a) : Statement(b)
{
  asserted = a;
}

void Assertion::resolveImpl()
{
  resolveExpr(asserted);
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
  auto enclosingStruct = enclosing->getMemberContext();
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
  id = nextSubrID++;
  allSubrs.push_back(this);
}

void Subroutine::resolveImpl()
{
  if(scope->parent->node.is<Block*>() && !type->pure)
  {
    errMsgLoc(this, "can't declare procedure in block scope");
  }
  type->resolve();
  for(auto arg : args)
  {
    //resolving argument variables just resolves their types
    arg->resolve();
  }
  if(type->returnType == primitives[Prim::VOID] &&
      (body->stmts.size() == 0 ||
      !dynamic_cast<Return*>(body->stmts.back())))
  {
    body->stmts.push_back(new Return(body));
  }
  //resolve the body
  body->resolve();
  //do additional checks for main()
  if(name == "main")
  {
    if(scope->parent != global->scope)
    {
      errMsgLoc(this, "main() must be in global scope");
    }
    if(type->pure)
    {
      errMsgLoc(this, "main() must be a procedure, not a function");
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
  id = nextExSubrID++;
  allExSubrs.push_back(this);
}

void ExternalSubroutine::resolveImpl()
{
  type->resolve();
  resolved = true;
}

Test::Test(Scope* s, Block* b) : scope(s), run(b)
{
  tests.push_back(this);
}

void Test::resolveImpl()
{
  run->resolve();
  resolved = true;
}

