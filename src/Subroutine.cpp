#include "Subroutine.hpp"
#include "Variable.hpp"
#include <algorithm>

using std::find;

static int nextSubrID = 0;
//static int nextExSubrID = 0;

Subroutine* mainSubr = nullptr;
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
  resolveExpr(rvalue);
  //Default-initialized local variables produce an assignment
  //with a null rvalue - use the default value for the type
  if(!lvalue->assignable())
  {
    errMsgLoc(this, "left-hand side of assignment is immutable");
  }
  if(!lvalue->type->canConvert(rvalue->type))
  {
    errMsgLoc(this, "cannot convert from " << rvalue->type->getName() << " to " << lvalue->type->getName());
  }
  if(!typesSame(lvalue->type, rvalue->type))
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
  if(init)
    init->resolve();
  if(condition)
  {
    condition->resolve();
    if(!typesSame(condition->type, primitives[Prim::BOOL]))
    {
      errMsgLoc(condition, "C-style for loop condition must be a bool");
    }
  }
  if(increment)
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
  iter = new Variable(iters.back(), new ElemExprType(arr, iters.size() - 1), outer);
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
  if(arrType->dims < (int) counters.size())
  {
    errMsgLoc(this, "In for-loop over array, requested " << counters.size() <<
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
  if(!typesSame(counter->type, begin->type))
  {
    begin = new Converted(begin, counter->type);
    begin->resolve();
  }
  resolveExpr(end);
  if(!typesSame(counter->type, end->type))
  {
    end = new Converted(end, counter->type);
    end->resolve();
  }
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
  if(!typesSame(condition->type, primitives[Prim::BOOL]))
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
  elseBody = nullptr;
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
  auto ut = dynamic_cast<UnionType*>(canonicalize(matched->type));
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
    bool foundInUnion = false;
    for(auto op : ut->options)
    {
      if(typesSame(t, op))
      {
        foundInUnion = true;
        break;
      }
    }
    if(!foundInUnion)
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

Switch::Switch(Block* b, Expression* s, Block* stmtBlock)
  : Statement(b)
{
  switched = s;
  block = stmtBlock;
}

void Switch::resolveImpl()
{
  resolveExpr(switched);
  //resolve case values and make sure they can convert to 
  for(auto& caseVal : caseValues)
  {
    //intercept special case of enum values, without preceding enum name.
    if(!caseVal->resolved)
    {
      auto switchedEnum = dynamic_cast<EnumType*>(switched->type);
      UnresolvedExpr::setShortcutEnum(switchedEnum);
      resolveExpr(caseVal);
      UnresolvedExpr::clearShortcutEnum();
    }
    if(!switched->type->canConvert(caseVal->type))
    {
      errMsgLoc(caseVal, "case value type incompatible with switch value type");
    }
    else if(!typesSame(switched->type, caseVal->type))
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
    resolveExpr(value);
  }
  //make sure value can be converted to enclosing subroutine's return type
  auto subrRetType = block->subr->type->returnType;
  if(typesSame(subrRetType, primitives[Prim::VOID]))
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
  else if(!typesSame(subrRetType, value->type))
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
  if(!typesSame(asserted->type, primitives[Prim::BOOL]))
  {
    errMsgLoc(this, "asserted value has non-bool type " << asserted->type->getName());
  }
  resolved = true;
}

/******************/
/* SubroutineDecl */
/******************/

SubroutineDecl::SubroutineDecl(string n, Scope* s, bool pure, bool explicitStatic)
  : name(n), scope(s), isPure(pure), owner(nullptr)
{
  //Determine the "this" type
  auto structContext = scope->parent->getMemberContext();
  if(!explicitStatic && structContext)
  {
    owner = structContext;
  }
  else if(explicitStatic && !structContext)
  {
    errMsgLoc(this, (isPure ? "func" : "proc") << " declared static, but not inside a struct");
  }
}

SubrBase* SubroutineDecl::match(vector<Type*>& params, bool* exact)
{
  INTERNAL_ASSERT(this->resolved);
  auto typeMatches =
  [&](CallableType* ct, bool allowConv) -> bool
  {
    auto& cp = ct->paramTypes;
    if(params.size() != cp.size())
      return false;
    for(size_t i = 0; i < cp.size(); i++)
    {
      if(allowConv)
      {
        if(!cp[i]->canConvert(params[i]))
          return false;
      }
      else
      {
        if(!typesSame(cp[i], params[i]))
          return false;
      }
    }
    return true;
  };
  for(int allowConv = 0; allowConv < 2; allowConv++)
  {
    for(auto s : overloads)
    {
      if(typeMatches(s->type, allowConv))
        return s;
    }
  }
  return nullptr;
}

SubrBase* SubroutineDecl::match(CallableType* ct)
{
  INTERNAL_ASSERT(this->resolved);
  for(auto o : overloads)
  {
    if(typesSame(o->type, ct))
      return o;
  }
  return nullptr;
}

void SubroutineDecl::resolveImpl()
{
  //Resolve just the types of each member
  for(auto o : overloads)
    o->type->resolve();
  //Hash the parameters from each type signature: if there's a conflict
  //(extremely unlikely) then fall back to using typesSame() on every pair.
  bool collision = false;
  unordered_set<size_t> hashes;
  for(auto o : overloads)
  {
    size_t thash = o->type->hash();
    auto iter = hashes.find(thash);
    if(iter == hashes.end())
      hashes.insert(thash);
    else
    {
      collision = true;
      break;
    }
  }
  if(collision)
  {
    //fall back to naive pairwise comparison
    for(size_t i = 1; i < overloads.size(); i++)
    {
      for(size_t j = 0; j < i; j++)
      {
        SubrBase* s1 = overloads[i];
        SubrBase* s2 = overloads[j];
        if(typesSame(s1->type, s2->type))
        {
          errMsg("Conflicting overloads for " << name << ": one at " << s1->printLocation() << " and the other at " << s2->printLocation());
        }
      }
    }
  }
  //now finish resolving the full body of the subroutines
  for(auto o : overloads)
    o->resolve();
  resolved = true;
}

/**************/
/* Subroutine */
/**************/

Subroutine::Subroutine(SubroutineDecl* d)
  : SubrBase(d)
{
  //Create the scope for parameters.
  scope = new Scope(d->scope, this);
  //Body will be a sub-scope of that)
  body = new Block(this);
  id = nextSubrID++;
}

void Subroutine::setSignature(Type* retType, vector<Variable*>& parsedParams)
{
  params = parsedParams;
  vector<Type*> paramTypes;
  for(auto p : params)
    paramTypes.push_back(p->type);
  if(decl->owner)
    type = new CallableType(decl->isPure, decl->owner, retType, paramTypes);
  else
    type = new CallableType(decl->isPure, retType, paramTypes);
}

void Subroutine::resolveImpl()
{
  INTERNAL_ASSERT(type->resolved);
  if(scope->parent->node.is<Block*>() && !type->pure)
  {
    errMsgLoc(this, "can't declare procedure in block scope");
  }
  for(auto param : params)
  {
    //resolving the param variables just resolves their types
    param->resolve();
  }
  //pretend this is resolved, so that recursive calls can resolve
  //(the type and params are resolved, so CallExprs can resolve successfully)
  resolved = true;
  if(typesSame(type->returnType, primitives[Prim::VOID]) &&
      (body->stmts.size() == 0 ||
      !dynamic_cast<Return*>(body->stmts.back())))
  {
    body->stmts.push_back(new Return(body));
  }
  //resolve the body
  body->resolve();
  //do additional checks for main()
  if(name() == "main")
  {
    if(mainSubr)
    {
      errMsgLoc(this, "main() can't be overloaded (previous definition at " << mainSubr->printLocation() << ")");
    }
    if(scope->parent != global->scope)
    {
      errMsgLoc(this, "main() must be in global scope");
    }
    if(type->pure)
    {
      errMsgLoc(this, "main() must be a procedure, not a function");
    }
    if(!typesSame(type->returnType, primitives[Prim::VOID]) &&
        !typesSame(type->returnType, primitives[Prim::INT]))
    {
      errMsgLoc(this, "main() must return void or int");
    }
    if(type->paramTypes.size() > 1 ||
        (type->paramTypes.size() == 1 &&
         !typesSame(type->paramTypes[0], getArrayType(primitives[Prim::CHAR], 2))))
    {
      cout << "Main's parameter has type " << type->paramTypes[0]->getName() << '\n';
      cout << "Expect it to have: " << getArrayType(primitives[Prim::CHAR], 2)->getName() << '\n';
      errMsgLoc(this, "main() must have either no parameters or just string[]");
    }
    mainSubr = this;
  }
}

ExternalSubroutine::ExternalSubroutine(
    SubroutineDecl* sd,
    Scope* s,
    string name,
    Type* returnType,
    vector<Type*>& ptypes,
    vector<string>& pnames,
    vector<bool>& borrow,
    string& code)
  : SubrBase(sd)
{
  //TODO
  INTERNAL_ERROR;
}

void ExternalSubroutine::resolveImpl()
{
  INTERNAL_ERROR;
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

