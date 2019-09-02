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
  current.thisExpr = thisExpr;
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
  if(auto assign = dynamic_cast<Assign*>(stmt))
  {
    if(auto va = dynamic_cast<VarExpr*>(assign->lvalue))
      assignVar(va->var, evaluate(assign->rvalue);
    else
      assignVar(evaluate(assign->lvalue), evaluate(assign->rvalue));
  }
  else if(auto block = dynamic_cast<Block*>(stmt))
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
    evaluate(call->eval);
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
    Expression* operand = evaluate(ua->expr);
    switch(ua->op)
    {
      case LNOT:
        {
          //Logical NOT
          BoolConstant* bc = dynamic_cast<BoolConstant*>(operand);
          INTERNAL_ASSERT(bc);
          return new BoolConstant(!bc->value);
        }
      case BNOT:
        {
          //Bitwise NOT: only applies to integers
          IntConstant* ic = dynamic_cast<IntConstant*>(operand->copy());
          INTERNAL_ASSERT(ic);
          if(ic->isSigned())
            ic->sval = ~ic->sval;
          else
            ic->uval = ~ic->uval;
          return ic;
        }
      case SUB:
        {
          //Negation: applies to integers and floats
          Expression* copy = operand->copy();
          if(auto ic = dynamic_cast<IntegerConstant*>(copy))
          {
            if(ic->isSigned())
              ic->sval = -ic->sval;
            else
              ic->uval = -ic->uval;
          }
          else if(auto fc = dynamic_cast<FloatConstant*>(copy))
          {
            if(fc->isDoublePrec())
              fc->dp = -fc->dp;
            else
              fc->sp = -fc->sp;
          }
          return ic;
        }
      default:
        INTERNAL_ERROR;
    }
    return nullptr;
  }
  else if(auto ba = dynamic_cast<BinaryArith*>(e))
  {
    //first, intercept short-circuit evaluation cases (logical AND/OR)
    //need to fold only the LHS, then replace value immediately if it short-circuits
    //NOTE: since constant folds never have side effects, this change never
    //affects program semantics but may allow folding when otherwise impossible
    Expression* lhs = evaluate(ba->lhs);
    Expression* rhs = evaluate(ba->rhs);
    int op = ba->op;
    //Comparison operations easy because
    //all constant Expressions support comparison,
    //with semantics matching the language
    switch(op)
    {
      //note: ordering operators are not implemented for all Expression subclasses,
      //but they are implemented for all constant types where they're allowed in syntax
      case CMPEQ:
        return new BoolConstant(*lhs == *rhs);
      case CMPNEQ:
        return new BoolConstant(!(*lhs == *rhs));
      case CMPL:
        return new BoolConstant(*lhs < *rhs);
      case CMPG:
        return new BoolConstant(*rhs < *lhs);
      case CMPLE:
        return new BoolConstant(!(*rhs < *lhs));
      case CMPGE:
        return new BoolConstant(!(*lhs < *rhs));
      default:;
    }
    if(op == PLUS)
    {
      //handle array concat, prepend and append operations (not numeric + yet)
      CompoundLiteral* compoundLHS = dynamic_cast<CompoundLiteral*>(lhs);
      CompoundLiteral* compoundRHS = dynamic_cast<CompoundLiteral*>(rhs);
      if((compoundLHS || compoundRHS) && oversize)
      {
        //+ on arrays always increases object size, so don't fold with oversized values
        return nullptr;
      }
      if(compoundLHS && compoundRHS)
      {
        vector<Expression*> resultMembers(compoundLHS->members.size() + compoundRHS->members.size());
        for(size_t i = 0; i < compoundLHS->members.size(); i++)
          resultMembers[i] = compoundLHS->members[i];
        for(size_t i = 0; i < compoundRHS->members.size(); i++)
          resultMembers[i + compoundLHS->members.size()] = compoundRHS->members[i];
        CompoundLiteral* result = new CompoundLiteral(resultMembers);
        result->resolve();
        return result;
      }
      else if(compoundLHS)
      {
        //array append
        vector<Expression*> resultMembers = compoundLHS->members;
        resultMembers.push_back(rhs);
        CompoundLiteral* result = new CompoundLiteral(resultMembers);
        result->resolve();
        return result;
      }
      else if(compoundRHS)
      {
        //array prepend
        vector<Expression*> resultMembers(1 + compoundRHS->members.size());
        resultMembers[0] = lhs;
        for(size_t i = 0; i < compoundRHS->members.size(); i++)
        {
          resultMembers[i + 1] = compoundRHS->members[i];
        }
        CompoundLiteral* result = new CompoundLiteral(resultMembers);
        result->resolve();
        return result;
      }
    }
    else if(op == LOR)
    {
      auto lhsBool = (BoolConstant*) lhs;
      auto rhsBool = (BoolConstant*) rhs;
      return new BoolConstant(lhsBool->value || rhsBool->value);
    }
    else if(op == LAND)
    {
      auto lhsBool = (BoolConstant*) lhs;
      auto rhsBool = (BoolConstant*) rhs;
      return new BoolConstant(lhsBool->value && rhsBool->value);
    }
    //all other binary ops are numerical operations between two ints or two floats
    FloatConstant* lhsFloat = dynamic_cast<FloatConstant*>(lhs);
    FloatConstant* rhsFloat = dynamic_cast<FloatConstant*>(rhs);
    IntConstant* lhsInt = dynamic_cast<IntConstant*>(lhs);
    IntConstant* rhsInt = dynamic_cast<IntConstant*>(rhs);
    bool useFloat = lhsFloat && rhsFloat;
    bool useInt = lhsInt && rhsInt;
    if(useFloat)
      return lhsFloat->binOp(op, rhsFloat);
    INTERNAL_ASSERT(useInt);
    return lhsInt->binOp(op, rhsInt);
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
    Expression* group = evaluate(ind->group);
    Expression* index = evaluate(ind->index);
    //arrays are represented as either CompoundLiteral or StringConstant
    auto cl = dynamic_cast<CompoundLiteral*>(group);
    auto sc = dynamic_cast<StringConstant*>(group);
    if(cl || sc)
    {
      //an array, so index should be an integer
      IntConstant* ic = dynamic_cast<IntConstant*>(index);
      INTERNAL_ASSERT(ic);
      unsigned ord = 0;
      if(ic->isSigned())
      {
        if(ic->sval < 0)
          errMsgLoc(ind, "negative array index");
        ord = ic->sval;
      }
      else
        ord = ic->uval;
      if(cl)
      {
        if(ord >= cl->members.size())
          errMsgLoc(ind, "array index out of bounds");
        return cl->members[ord];
      }
      else
      {
        if(ord >= sc->value.length())
          errMsgLoc(ind, "string index out of bounds");
        return new CharConstant(sc->value[ord]);
      }
    }
    else if(auto mc = dynamic_cast<MapConstant*>(group))
    {
      MapType* mapType = (MapType*) mt->type;
      //if key (index) is not already in the map, insert it and default-initialize the value
      if(mc->values.find(group) == mc->values.end())
        mc->values[group] = mapType->value->getDefaultValue();
      return mc->values[group];
    }
    INTERNAL_ERROR;
  }
  else if(auto call = dynamic_cast<CallExpr*>(e))
  {
    //evaluate callable, then args in order
    auto callable = evaluate(call->callable);
    Expression* callTarget = nullptr;
    if(auto subExpr = dynamic_cast<SubroutineExpr*>(callable))
      callTarget = subExpr;
    else
      callTarget = evaluate(callable);
    vector<Expression*> args;
    for(auto a : eval->args)
      args.push_back(evaluate(a));
    //All "constant" callables must be SubroutineExpr,
    auto subExpr = dynamic_cast<SubroutineExpr*>(callable);
    INTERNAL_ASSERT(!subExpr);
    if(subExpr->subr)
      callSubr(subExpr->subr, args, subExpr->thisObject);
    else
      callExtern(subExpr->exSubr, args);
  }
  else if(auto v = dynamic_cast<VarExpr*>(e))
  {
    return readVar(v);
  }
  else if(auto sm = dynamic_cast<StructMem*>(e))
  {
    //All structs are represented as CompoundLiteral
    CompoundLiteral* base = dynamic_cast<CompoundLiteral*>(evaluate(sm->base));
    INTERNAL_ASSERT(base);
    StructType* structType = dynamic_cast<StructType*>(base->type);
    INTERNAL_ASSERT(structType);
    auto& dataMems = structType->members;
    if(sm->member.is<Variable*>())
    {
      for(size_t i = 0; i < dataMems.size(); i++)
      {
        if(dataMems[i] == sm->member.get<Variable*>())
          return base->members[i];
      }
      INTERNAL_ERROR;
    }
    else
    {
      //Member subroutine - return a SubroutineExpr
      return new SubroutineExpr(base, sm->member.get<Subroutine*>());
    }
    return nullptr;
  }
  else if(auto na = dynamic_cast<NewArray*>(e))
  {
  }
  else if(auto al = dynamic_cast<ArrayLength*>(e))
  {
    //note: the type of this expression is always "long"
    Expression* arr = evaluate(al->array);
    if(auto cl = dynamic_cast<CompoundLiteral*>(arr))
    {
      return new IntConstant((int64_t) cl->members.size());
    }
    else if(auto sc = dynamic_cast<StringConstant*>(arr))
    {
      return new IntConstant((int64_t) sc->value.length());
    }
    INTERNAL_ERROR;
  }
  else if(auto ie = dynamic_cast<IsExpr*>(e))
  {
    UnionConstant* uc = dynamic_cast<UnionConstant*>(evaluate(ie->base));
    INTERNAL_ASSERT(uc);
    return new BoolConstant(uc->optionIndex == ie->option);
  }
  else if(auto ae = dynamic_cast<AsExpr*>(e))
  {
    UnionConstant* uc = dynamic_cast<UnionConstant*>(evaluate(ae->base));
    INTERNAL_ASSERT(uc);
    if(uc->optionIndex != ae->option)
      errMsgLoc(ae, "union value does not have the type expected by \"as\"");
    return ae->base;
  }
  else if(auto te = dynamic_cast<ThisExpr*>(e))
  {
    return frames.back().thisExpr;
  }
  else if(auto conv = dynamic_cast<Converted*>(e))
  {
  }
  else if(auto ee = dynamic_cast<EnumExpr*>(e))
  {
  }
  INTERNAL_ERROR;
  return nullptr;
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

