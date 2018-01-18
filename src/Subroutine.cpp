#include "Subroutine.hpp"
#include "Variable.hpp"

using namespace TypeSystem;

extern map<Parser::Block*, BlockScope*> blockScopes;
extern bool programHasMain;
extern ModuleScope* global;

vector<Test*> Test::tests;

//Block which is body of subroutine
Block::Block(Parser::Block* b, BlockScope* s, Subroutine* sub) : scope(s)
{
  this->subr = sub;
  //to get func, walk up scope tree from s
  //until a SubroutineScope for a function is reached
  this->funcScope = nullptr;
  for(Scope* iter = s; iter; iter = iter->parent)
  {
    SubroutineScope* ss = dynamic_cast<SubroutineScope*>(iter);
    if(ss && ss->subr->type->pure)
    {
      this->funcScope = ss;
      break;
    }
  }
  this->loop = None();
  this->breakable = None();
  //don't add statements until 2nd middle end pass
}

//Block which is used as a regular statement in another block
Block::Block(Parser::Block* b, BlockScope* s, Block* parent) : scope(s)
{
  this->subr = parent->subr;
  this->funcScope = parent->funcScope;
  this->loop = parent->loop;
  this->breakable = parent->breakable;
}

//Internal-use ctor: create a block without any corresponding parsed Block
Block::Block(BlockScope* s, Block* parent) : scope(s)
{
  this->subr = parent->subr;
  this->funcScope = parent->funcScope;
  this->loop = parent->loop;
  this->breakable = parent->breakable;
  //don't add any statements yet
}

//Block which is a for loop body
Block::Block(Parser::For* forAST, For* f, BlockScope* s, Block* parent) : scope(s)
{
  this->subr = parent->subr;
  this->funcScope = parent->funcScope;
  this->loop = f;
  this->breakable = f;
}

//Block which is a while loop body
Block::Block(Parser::While* whileAST, While* w, BlockScope* s, Block* parent) : scope(s)
{
  this->subr = parent->subr;
  this->funcScope = parent->funcScope;
  this->loop = w;
  this->breakable = w;
}

Block::Block(BlockScope* s) : scope(s)
{
  this->subr = nullptr;
  this->funcScope = nullptr;
  this->loop = None();
  this->breakable = None();
}

void Block::addStatements(Parser::Block* ast)
{
  for(auto stmt : ast->statements)
  {
    auto s = createStatement(this, stmt);
    if(s)
    {
      stmts.push_back(s);
    }
  }
}

void Block::check()
{
  if(funcScope)
  {
    checkPurity(funcScope);
  }
}

void Block::checkPurity(Scope* s)
{
  for(auto stmt : stmts)
  {
    stmt->checkPurity(s);
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
    Block* newBlock = new Block(block, blockScopes[block], b);
    newBlock->addStatements(block);
    return newBlock;
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
    return new Switch(stmt->s.get<Parser::Switch*>(), b);
  }
  else if(stmt->s.is<Parser::Match*>())
  {
    return new Match(stmt->s.get<Parser::Match*>(), b);
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
    return new Assign(newVar, getExpression(s, vd->val));
  }
  return nullptr;
}

Assign::Assign(Parser::VarAssign* va, Scope* s)
{
  lvalue = getExpression(s, va->target);
  rvalue = getExpression(s, va->rhs);
  commonCtor();
}

Assign::Assign(Variable* target, Expression* e)
{
  lvalue = new VarExpr(target);
  rvalue = e;
  commonCtor();
}

Assign::Assign(Indexed* target, Expression* e)
{
  lvalue = target;
  rvalue = e;
  commonCtor();
}

void Assign::commonCtor()
{
  if(!lvalue->assignable())
  {
    ERR_MSG("cannot assign to that expression");
  }
  //check for special case of map index as lvalue
  //the type of this would normally be (Key | Error) but
  //as an lvalue is just Key (backend expects this)
  bool lvalueMapIndex = false;
  if(auto indexed = dynamic_cast<Indexed*>(lvalue))
  {
    if(indexed->group->type->isMap())
      lvalueMapIndex = true;
  }
  if(lvalue->type != rvalue->type && !lvalueMapIndex)
  {
    //must explicitly convert
    rvalue = new Converted(rvalue, lvalue->type);
  }
}

void Assign::checkPurity(Scope* s)
{
  if(!lvalue->pureWithin(s))
  {
    ERR_MSG("in function, assignment lvalue lives outside fn scope");
  }
  if(!rvalue->pureWithin(s))
  {
    ERR_MSG("in function, assigned value isn't pure");
  }
}

CallStmt::CallStmt(Parser::Expr12* call, BlockScope* s)
{
  eval = (CallExpr*) getExpression(s, call);
}

void CallStmt::checkPurity(Scope* s)
{
  if(!eval->pureWithin(s))
  {
    ERR_MSG("call isn't allowed in function");
  }
}

For::For(Parser::For* f, Block* b)
{
  BlockScope* loopScope = blockScopes[f->body];
  loopBlock = new Block(f, this, loopScope, b);
  auto enclosing = loopScope->parent;
  //constants that are helpful for generating loops
  Expression* zero = new IntLiteral(0ULL);
  Expression* one = new IntLiteral(1ULL);
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
      if(condition->type != TypeSystem::primitives[Parser::TypeNT::BOOL])
      {
        ERR_MSG("condition in C-style for loop must be a boolean expression");
      }
    }
    if(fc->incr)
    {
      increment = createStatement(loopBlock, fc->incr);
    }
    loopBlock->addStatements(f->body);
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
    //how many loops to generate
    int loops = foa->tup.size() - 1;
    if(arrType->dims < loops)
    {
      ERR_MSG("for-array tuple has more counters than array has dimensions");
    }
    //generate one for loop (including this one) as the body for each dimension
    //and the "it" value in the innermost loop
    For* dimLoop = this;
    Block* dimBlock = loopBlock;
    vector<Variable*> counters;
    for(int i = 0; i < loops; i++)
    {
      if(i > 0)
      {
        //construct next loop's scope as child of loopScope
        BlockScope* nextScope = new BlockScope(loopScope);
        For* nextFor = new For;
        Block* nextBlock = new Block(nextScope, dimBlock);
        //break goes with the outermost loop, but continue goes with the innermost
        nextBlock->breakable = this;
        nextBlock->loop = nextFor;
        nextFor->loopBlock = nextBlock;
        //have the outer loop run the inner loop
        dimBlock->stmts.push_back(nextFor);
        dimLoop = nextFor;
        dimBlock = nextBlock;
      }
      //generate counter for dimension i (adding it to scope implicitly catches shadowing errors)
      Variable* counter = new Variable(dimBlock->scope, foa->tup[i], TypeSystem::primitives[Parser::TypeNT::INT]);
      dimBlock->scope->addName(counter);
      counters.push_back(counter);
      VarExpr* counterExpr = new VarExpr(counter);
      //in order to get length expression for loop condition, get array expression
      Expression* subArr = arr;
      for(int j = 0; j < i; j++)
      {
        subArr = new Indexed(subArr, new VarExpr(counters[j]));
      }
      dimLoop->init = new Assign(counter, zero);
      dimLoop->condition = new BinaryArith(counterExpr, CMPL, new ArrayLength(subArr));
      dimLoop->increment = new Assign(counter, new BinaryArith(counterExpr, PLUS, one));
      //now create the "iter" value if this is the innermost loop
      if(i == loops - 1)
      {
        Type* iterType = getArrayType(arrType->elem, arrType->dims - loops);
        Variable* iterValue = new Variable(dimBlock->scope, foa->tup.back(), iterType);
        dimBlock->scope->addName(iterValue);
        //create the assignment to iterValue as first statement in innermost loop
        dimBlock->stmts.push_back(new Assign(iterValue, new Indexed(subArr, counterExpr)));
        //Then add all the actual statements from the body of foa
        //Even though middle end originally tied it to the
        //outermost loop, the statements go in the innermost
        dimBlock->addStatements(f->body);
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
    Variable* counter = new Variable(loopScope, fr->name, counterType);
    loopScope->addName(counter);
    init = new Assign(counter, start);
    condition = new BinaryArith(new VarExpr(counter), CMPL, end);
    increment = new Assign(counter, new BinaryArith(new VarExpr(counter), PLUS, one));
    loopBlock->addStatements(f->body);
  }
  else
  {
    INTERNAL_ERROR;
  }
}

void For::checkPurity(Scope* s)
{
  init->checkPurity(s);
  if(!condition->pureWithin(s))
  {
    ERR_MSG("for loop in function has non-pure condition");
  }
  increment->checkPurity(s);
  loopBlock->checkPurity(s);
}

While::While(Parser::While* w, Block* b)
{
  auto enclosing = b->scope;
  condition = getExpression(enclosing, w->cond);
  if(condition->type != TypeSystem::primitives[Parser::TypeNT::BOOL])
  {
    ERR_MSG("while loop condition must be a bool");
  }
  loopBlock = new Block(w, this, blockScopes[w->body], b);
  loopBlock->addStatements(w->body);
}

void While::checkPurity(Scope* s)
{
  if(!condition->pureWithin(s))
  {
    ERR_MSG("while loop in function has non-pure condition");
  }
  loopBlock->checkPurity(s);
}

If::If(Parser::If* i, Block* b)
{
  condition = getExpression(b->scope, i->cond);
  if(condition->type != TypeSystem::primitives[Parser::TypeNT::BOOL])
  {
    ERR_MSG("if statement condition must be a bool");
  }
  body = createStatement(b, i->ifBody);
}

void If::checkPurity(Scope* s)
{
  if(!condition->pureWithin(s))
  {
    ERR_MSG("if statement in function has non-pure condition");
  }
}

IfElse::IfElse(Parser::If* i, Block* b)
{
  condition = getExpression(b->scope, i->cond);
  if(condition->type != TypeSystem::primitives[Parser::TypeNT::BOOL])
  {
    ERR_MSG("if/else condition must be a bool");
  }
  trueBody = createStatement(b, i->ifBody);
  falseBody = createStatement(b, i->elseBody);
}

void IfElse::checkPurity(Scope* s)
{
  if(!condition->pureWithin(s))
  {
    ERR_MSG("if/else in function has non-pure condition");
  }
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

void Match::checkPurity(Scope* s)
{
  if(!matched->pureWithin(s))
  {
    ERR_MSG("type-matched expression in match statement violates purity");
  }
  for(auto c : cases)
  {
    c->checkPurity(s);
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

void Switch::checkPurity(Scope* s)
{
  if(!switched->pureWithin(s))
  {
    ERR_MSG("switched value in switch statement violates purity");
  }
  for(auto cval : caseValues)
  {
    if(!cval->pureWithin(s))
    {
      ERR_MSG("switch statement case value violates purity");
    }
  }
  block->checkPurity(s);
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

void Return::checkPurity(Scope* s)
{
  if(value && !value->pureWithin(s))
  {
    ERR_MSG("return value violates purity");
  }
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

void Print::checkPurity(Scope* s)
{
  if(s)
  {
    ERR_MSG("print() has side effects and can't be used in a function");
  }
}

Assertion::Assertion(Parser::Assertion* as, BlockScope* s)
{
  asserted = getExpression(s, as->expr);
}

void Assertion::checkPurity(Scope* s)
{
  if(!asserted->pureWithin(s))
  {
    ERR_MSG("asserted value violates purity");
  }
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
    if(type->returnType != primitives[Parser::TypeNT::VOID] &&
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

