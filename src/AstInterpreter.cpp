#include "AstInterpreter.hpp"

StackFrame::StackFrame()
{
  rv = nullptr;
}

Interpreter::Interpreter(Subroutine* subr, vector<Expression*> args)
{
  returning = false;
  callSubr(subr, args);
}

Expression* Interpreter::callSubr(Subroutine* subr, vector<Expression*> args, Expression* thisExpr)
{
  returning = false;
  //push stack frame
  frames.emplace_back();
  Frame& current = frames.back();
  //Execute statements in linear sequence.
  //If a return is encountered, execute() returns and
  //the return value will be placed in the frame rv
  for(auto s : subr->stmts)
  {
    execute(s);
    if(current->returning)
    {
      Expression* rv = current.rv;
      frames.pop_back();
      return rv;
    }
  }
  frames.pop_back();
  //implicit return, so there is no return value.
  //Check that the subroutine doesn't return anything,
  //since the resolved AST can't check for missing returns
  if(!subr->type->returnType->isSimple())
  {
    errMsgLoc(subr, "interpreter reached end of subroutine without a return value");
  }
  return nullptr;
}

Expression* Interpreter::callExtern(ExternalSubroutine* exSubr, vector<Expression*> args)
{
  //TODO: lazily load correct dynamic library, put arguments in correct binary format, and call
}

void Interpreter::execute(Statement* stmt)
{
  if(breaking || continuing || returning)
    return;
  if(auto block = dynamic_cast<Block*>(stmt))
  {
    for(auto stmt : block->stmts)
    {
      execute(stmt);
      if(breaking || continuing || returning)
        return;
    }
  }
  else if(auto call = dynamic_cast<CallStmt*>(stmt))
  {
    CallExpr* eval = call->eval;
    auto callable = eval->callable;
    Expression* callTarget = nullptr;
    if(auto subExpr = dynamic_cast<SubroutineExpr*>(callable))
      callTarget = subExpr;
    else
      callTarget = evaluate(callable);
    vector<Expression*> args;
    for(auto a : eval->args)
      args.push_back(evaluate(a));
    auto subExpr = dynamic_cast<SubroutineExpr*>(callable);
    INTERNAL_ASSERT(!subExpr);
    if(subExpr->subr)
      callSubr(subExpr->subr, args, subExpr->thisObject);
    else
      callExtern(subExpr->exSubr, args);
  }
  else if(auto fc = dynamic_cast<ForC*>(stmt))
  {
    //Initialize the loop
    if(fc->init)
      execute(fc->init);
    while(true)
    {
      //Evaluate the condition
      BoolConstant* cond = dynamic_cast<BoolConstant*>(evaluate(fc->condition));
      INTERNAL_ASSERT(cond);
      if(!cond->value)
        break;
      execute(fc->inner);
      if(breaking)
      {
        breaking = false;
        break;
      }
      else if(continuing)
        continuing = false;
      else if(returning)
        break;
      //"continue" is implicit: body execution breaks immediately and the loop continues
      if(fc->increment)
        execute(fc->increment);
    }
  }
  else if(auto fr = dynamic_cast<ForRange*>(stmt))
  {
    //Create a statement to initialize counter, and another to increment
    VarExpr* ve = new VarExpr(fr->counter);
    Statement* init = new Assign(fr->block, ve, ASSIGN, fr->begin);
    Statement* incr = new Assign(fr->block, ve, INC);
    Expression* cond = new BinaryArith(ve, CMPL, fr->end);
    init->resolve();
    incr->resolve();
    cond->resolve();
    while(true)
    {
      BoolConstant* cond = dynamic_cast<BoolConstant*>(evaluate(cond));
      INTERNAL_ASSERT(cond);
      if(!cond->value)
        break;
      execute(fc->inner);
      if(breaking)
      {
        breaking = false;
        break;
      }
      else if(continuing)
        continuing = false;
      else if(returning)
        break;
      execute(incr);
    }
  }
  else if(auto fa = dynamic_cast<ForArray*>(stmt))
  {
    Expression* zero = new IntLiteral(0, primitives[Prim::LONG]);
    Expression* iter = new VarExpr(fa->iter);
    zero->resolve();
    int dims = fa->counters.size();
    vector<Expression*> counterExprs;
    for(auto counter : fa->counters)
      counterExprs.push_back(new VarExpr(counter));
    //A ragged array is easily iterated using DFS over a tree.
    //Use a stack to store the nodes which must still be visited.
    //zero out first counter
    vector<Assign*> counterZero;
    for(int i = 0; i < dims; i++)
    {
      counterIncr.push_back(new Assign(counterExprs[0], INC));
      counterIncr.back()->resolve();
    }
    vector<int> indices(dims, 0);
    stack<pair<Expression*, depth>> toVisit;
    toVisit.emplace_back(evaluate(fa->arr), 0);
    while(!toVisit.empty())
    {
      Expression* visit;
      int depth;
      visit = toVisit.top().get<0>();
      depth = toVisit.top().get<1>();
      toVisit.pop();
      if(depth == dims - 1)
      {
        //innermost dimension, assign iter's value
        assignVar(fa->iter, visit);
      }
      else
      {
        //push elements of visit in reverse order
        if(auto cl = dynamic_cast<CompoundLiteral*>(visit))
          for(int i = cl->members.size() - 1; i >= 0; i--)
            toVisit.emplace_back(cl->members[i], depth + 1);
        else
        {
          StringConstant* sc = dynamic_cast<StringConstant*>(visit);
          INTERNAL_ASSERT(sc);
          string val = sc->value;
          for(int i = val.length() - 1; i >= 0; i--)
            toVisit.emplace_back(new CharConstant(val[i]), depth + 1);
        }
        //zero out next dim's counter,
        assignVar(fa->counters[depth + 1], zero);
      }
      //execute the body
      execute(fa->inner);
      if(breaking)
      {
        breaking = false;
        break;
      }
      else if(continuing)
        continuing = false;
      else if(returning)
        break;
      //finally, increment the counter for the current dimension
      IntConstant* countVal = dynamic_cast<IntConstant*>(readVar(fa->counters[depth]));
      INTERNAL_ASSERT(countVal);
      //"long" is the type used for counters
      INTERNAL_ASSERT(countVal->isSigned());
      countVal->sval++;
    }
  }
  else if(auto w = dynamic_cast<While*>(stmt))
  {
    while(true)
    {
      BoolConstant* cond = dynamic_cast<BoolConstant*>(evaluate(w->condition));
      INTERNAL_ASSERT(cond);
      if(!cond->value)
        break;
      execute(w->body);
      if(breaking)
      {
        breaking = false;
        break;
      }
      else if(continuing)
        continuing = false;
      else if(returning)
        break;
    }
  }
  else if(auto i = dynamic_cast<If*>(stmt))
  {
    BoolConstant* cond = dynamic_cast<BoolConstant*>(evaluate(i->condition));
    INTERNAL_ASSERT(cond);
    if(cond->value)
      execute(i->body);
    else if(i->elseBody)
      execute(i->elseBody);
  }
  else if(auto ret = dynamic_cast<Return*>(stmt))
  {
    if(ret->value)
      rv = evaluate(ret->value);
    returning = true;
  }
  else if(auto brk = dynamic_cast<Break*>(stmt))
  {
    breaking = true;
  }
  else if(auto cont = dynamic_cast<Continue*>(stmt))
  {
    continuing = true;
  }
  else if(auto print = dynamic_cast<Print*>(stmt))
  {
    for(auto e : print->exprs)
      cout << evaluate(e);
  }
  else if(auto assertion = dynamic_cast<Assert*>(stmt))
  {
    BoolConstant* bc = dynamic_cast<BoolConstant*>(evaluate(assertion->asserted));
    assert(bc);
    if(!bc->value);
    {
      errMsgLoc(assertion, "Assertion failed: " << assertion->asserted);
    }
  }
  else if(auto sw = dynamic_cast<Switch*>(stmt))
  {
    Expression* switched = evaluate(sw->switched);
    //Run down list of cases, comparing value
    int label = sw->defaultPosition;
    for(size_t i = 0; i < sw->caseValues.size(); i++)
    {
      if(*switched == *sw->caseValues[i])
      {
        label = sw->caseLabels[i];
        break;
      }
    }
    //begin executing body at the proper position
    for(size_t i = label; i < sw->block->stmts.size(); i++)
    {
      execute(sw->block->stmts[j]);
      if(breaking)
      {
        breaking = false;
        return;
      }
      else if(continuing || returning)
        return;
    }
  }
  else if(auto ma = dynamic_cast<Match*>(stmt))
  {
    UnionConstant* uc = dynamic_cast<UnionConstant*>(evaluate(ma->matched));
    INTERNAL_ASSERT(uc);
    for(size_t i = 0; i < ma->types.size(); i++)
    {
      if(typesSame(uc->value->type, ma->types[i]))
      {
        assignVar(ma->caseVars[i], uc->value);
        execute(ma->cases[i]);
        if(breaking)
          breaking = false;
        break;
      }
    }
  }
  else
  {
    INTERNAL_ERROR;
  }
}

Expression* Interpreter::evaluate(Expression* e)
{
  if(e->constant())
  {
    //nothing to do
    return e;
  }
  if(auto ua = dynamic_cast<UnaryArith*>(e))
  {
  }
  else if(auto ba = dynamic_cast<BinaryArith*>(e))
  {
  }
  else if(auto cl = dynamic_cast<CompoundLiteral*>(e))
  {
    vector<Expression*> elems;
    for(auto e : cl->members)
      elems.push_back(evaluate(e));
    return new CompoundLiteral(elems);
  }
  else if(auto ind = dynamic_cast<Indexed*>(e))
  {
  }
  else if(auto call = dynamic_cast<CallExpr*>(e))
  {
  }
  else if(auto v = dynamic_cast<VarExpr*>(e))
  {
    return readVar(v);
  }
  else if(auto sm = dynamic_cast<StructMem*>(e))
  {
  }
  else if(auto na = dynamic_cast<NewArray*>(e))
  {
  }
  else if(auto al = dynamic_cast<ArrayLength*>(e))
  {
  }
  else if(auto ie = dynamic_cast<IsExpr*>(e))
  {
  }
  else if(auto ae = dynamic_cast<AsExpr*>(e))
  {
  }
  else if(
}

void Interpreter::assignVar(Variable* v, Expression* e)
{
  if(!e->constant())
    e = evaluate(e);
  //Only have to search in top stack frame, and globals
  if(globals.find(v) != globals.end())
  {
    globals[v] = e;
  }
  else
  {
    //a reference to local must be in the current frame
    frames.back().locals[v] = e;
  }
}

Expression* Interpreter::readVar(Variable* v)
{
  if(globals.find(v) != globals.end())
    return globals[v];
  return frames.back().locals[v];
}

