#include "ConstantProp.hpp"
#include "Variable.hpp"
#include "Expression.hpp"

using namespace IR;

//Max number of bytes in constant expressions
//(higher value increases compiler memory usage and code size,
//but gives more opportunities for constant folding)
const int maxConstantSize = 512;

struct LocalConstantSet
{

};

//Join operator for "ConstantVar" (used by dataflow analysis).
//associative/commutative
//
//Since undefined values are impossible, there is no need for a "top" value
ConstantVar constantMeet(ConstantVar& lhs, ConstantVar& rhs)
{
  //meet(c, c) = c
  //meet(c, d) = x
  //meet(x, _) = x
  //meet(?, _) = _
  if(lhs.val.is<Expression*>() && rhs.val.is<Expression*>())
  {
    Expression* l = lhs.val.get<Expression*>();
    Expression* r = rhs.val.get<Expression*>();
    if(*l != *r)
      return ConstantVar(NON_CONSTANT);
    else
      return ConstantVar(l);
  }
  else if(lhs.val.is<NonConstant>() || rhs.val.is<NonConstant>())
  {
    return ConstantVar(NON_CONSTANT);
  }
  else if(lhs.val.is<UndefinedVal>())
  {
    return rhs;
  }
  return lhs;
}

map<Variable*, ConstantVar> globalConstants;

void foldGlobals()
{
  //before the first pass, assume all globals are non-constant
  for(auto v : allVars)
  {
    if(v->isGlobal())
    {
      globalConstants[v] = ConstantVar();
    }
  }
  bool update = true;
  while(update)
  {
    update = false;
    for(auto& glob : globalConstants)
    {
      Variable* globVar = glob.first;
      //fold globVar's initial value (if possible)
      Expression* prev = globVar->initial;
      foldExpression(globVar->initial);
      if(globVar->initial->constant() &&
          prev != globVar->initial)
      {
        update = true;
        //can update global's constant status to a constant value
        glob.second = ConstantVar(globVar->initial);
      }
      //on subsequent sweeps, expressions using constant
      //global variables can be folded
    }
  }
}

//Remove constant status of global variables that get modified anywhere
//Will still use the folded value to initialize global,
//but can't use that value during folding anymore
void filterGlobalConstants()
{
  for(auto& s : IR::ir)
  {
    auto subr = s.second;
    for(auto stmt : subr->stmts)
    {
      auto outputs = stmt->getOutput();
      for(auto out : outputs)
      {
        auto writes = out->getWrites();
        for(auto w : writes)
        {
          if(w->isGlobal())
          {
            //there is an assignment to w,
            //so w might not always be a constant
            globalConstants[w] = ConstantVar();
          }
        }
      }
    }
  }
}

//Convert a constant expression to another type
//conv->value must already be folded and be constant
static Expression* convertConstant(Expression* value, Type* type)
{
  INTERNAL_ASSERT(value->constant());
  int option = -1;
  if(auto unionDst = dynamic_cast<UnionType*>(type))
  {
    for(size_t i = 0; i < unionDst->options.size(); i++)
    {
      if(unionDst->options[i] == value->type)
      {
        option = i;
        break;
      }
    }
    if(option < 0)
    {
      for(size_t i = 0; i < unionDst->options.size(); i++)
      {
        if(unionDst->options[i]->canConvert(value->type))
        {
          option = i;
          value = convertConstant(value, unionDst->options[i]);
          break;
        }
      }
    }
    INTERNAL_ASSERT(option >= 0);
    cout << "creating union constant of type " << unionDst->options[option]->getName() << ", value " << value << '\n';
    return new UnionConstant(value, unionDst->options[option], unionDst);
  }
  else if(auto intConst = dynamic_cast<IntConstant*>(value))
  {
    //do the conversion which tests for overflow and enum membership
    return intConst->convert(type);
  }
  else if(auto charConst = dynamic_cast<CharConstant*>(value))
  {
    //char is equivalent to an 8-bit unsigned for purposes of value conversion
    return new IntConstant((uint64_t) charConst->value);
  }
  else if(auto floatConst = dynamic_cast<FloatConstant*>(value))
  {
    return floatConst->convert(type);
  }
  else if(auto enumConst = dynamic_cast<EnumExpr*>(value))
  {
    if(enumConst->value->fitsS64)
    {
      //make a signed temporary int constant, then convert that
      //(this can't possible lose information)
      IntConstant asInt(enumConst->value->sval);
      return asInt.convert(type);
    }
    else
    {
      IntConstant asInt(enumConst->value->uval);
      return asInt.convert(type);
    }
  }
  //array/struct/tuple constants can be converted implicitly
  //to each other (all use CompoundLiteral) but individual
  //members (primitives) may need conversion
  else if(auto compLit = dynamic_cast<CompoundLiteral*>(value))
  {
    //attempt to fold all elements (can't proceed unless every
    //one is a constant)
    bool allConstant = true;
    for(auto& mem : compLit->members)
    {
      foldExpression(mem);
      allConstant = allConstant && mem->constant();
    }
    if(!allConstant)
      return compLit;
    if(auto st = dynamic_cast<StructType*>(type))
    {
      for(size_t i = 0; i < compLit->members.size(); i++)
      {
        if(compLit->members[i]->type != st->members[i]->type)
        {
          compLit->members[i] = convertConstant(
              compLit->members[i], st->members[i]->type);
        }
        //else: don't need to convert member
      }
    }
    else if(auto tt = dynamic_cast<TupleType*>(type))
    {
      for(size_t i = 0; i < compLit->members.size(); i++)
      {
        if(compLit->members[i]->type != tt->members[i])
        {
          compLit->members[i] = convertConstant(
              compLit->members[i], tt->members[i]);
        }
      }
    }
    else if(auto mt = dynamic_cast<MapType*>(type))
    {
      auto mc = new MapConstant;
      //add each key/value pair to the map
      for(size_t i = 0; i < compLit->members.size(); i++)
      {
        auto kv = dynamic_cast<CompoundLiteral*>(compLit->members[i]);
        INTERNAL_ASSERT(kv);
        Expression* key = kv->members[0];
        Expression* val = kv->members[1];
        if(key->type != mt->key)
          key = convertConstant(key, mt->key);
        if(val->type != mt->value)
          val = convertConstant(val, mt->value);
        mc->values[key] = val;
      }
    }
  }
  //????
  INTERNAL_ERROR;
  return nullptr;
}

//Evaluate a numerical binary arithmetic operation.
//Check for integer overflow and invalid operations (e.g. x / 0)
static Expression* evalBinOp(Expression*& lhs, int op, Expression*& rhs)
{
  foldExpression(lhs);
  foldExpression(rhs);
  if(!lhs->constant() || !rhs->constant())
    return nullptr;
  //Comparison operations easy because
  //all constant Expressions support comparison
  switch(op)
  {
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
    bool oversize =
      lhs->getConstantSize() >= maxConstantSize ||
      rhs->getConstantSize() >= maxConstantSize;
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
  //all other binary ops are numerical operations between two ints or two floats
  FloatConstant* lhsFloat = dynamic_cast<FloatConstant*>(lhs);
  FloatConstant* rhsFloat = dynamic_cast<FloatConstant*>(rhs);
  IntConstant* lhsInt = dynamic_cast<IntConstant*>(lhs);
  IntConstant* rhsInt = dynamic_cast<IntConstant*>(rhs);
  bool useFloat = lhsFloat && rhsFloat;
  bool useInt = lhsInt && rhsInt;
  INTERNAL_ASSERT(useFloat ^ useInt);
  if(useFloat)
    return lhsFloat->binOp(op, rhsFloat);
  return lhsInt->binOp(op, rhsInt);
}

static CompoundLiteral* createArray(uint64_t* dims, int ndims, Type* elem)
{
  vector<Expression*> elems;
  for(uint64_t i = 0; i < dims[0]; i++)
  {
    if(ndims == 1)
    {
      elems.push_back(elem->getDefaultValue());
    }
    else
    {
      elems.push_back(createArray(&dims[1], ndims - 1, elem));
    }
  }
  CompoundLiteral* cl = new CompoundLiteral(elems);
  cl->type = getArrayType(elem, ndims);
  cl->resolved = true;
  return cl;
}

//Try to fold an expression, bottom-up
//Can fold all constants in one pass
//
//After calling this, if expr->constant(),
//is guaranteed to be completely folded into a simple constant,
//or inputs were too big to fold
//
//Return true if any IR changes are made
//Set constant to true if expr is now, or was already, a constant
void foldExpression(Expression*& expr)
{
  if(expr->constant())
  {
    //only thing needed here is convert each string constant to char arrays
    if(auto str = dynamic_cast<StringConstant*>(expr))
    {
      vector<Expression*> chars;
      for(size_t i = 0; i < str->value.size(); i++)
      {
        chars.push_back(new CharConstant(str->value[i]));
      }
      CompoundLiteral* cl = new CompoundLiteral(chars);
      cl->type = str->type;
      cl->resolved = true;
      expr = cl;
    }
    return;
  }
  else if(auto ve = dynamic_cast<VarExpr*>(expr))
  {
    if(ve->var->isGlobal())
    {
      //check the global constant table
      auto& cv = globalConstants[ve->var];
      if(cv.val.is<Expression*>())
      {
        expr = cv.val.get<Expression*>();
      }
    }
    else if(ve->var->isLocal())
    {
      //look up the variable in local constant table,
      //and replace if possible
    }
  }
  else if(auto conv = dynamic_cast<Converted*>(expr))
  {
    foldExpression(conv->value);
    if(conv->value->constant())
      expr = convertConstant(conv->value, conv->type);
  }
  else if(auto binArith = dynamic_cast<BinaryArith*>(expr))
  {
    //evalBinOp returns null if expr not constant or is too big to evaluate
    Expression* result = evalBinOp(binArith->lhs, binArith->op, binArith->rhs);
    if(result)
      expr = result;
  }
  else if(auto unaryArith = dynamic_cast<UnaryArith*>(expr))
  {
    foldExpression(unaryArith->expr);
    if(unaryArith->expr->constant())
    {
      if(unaryArith->op == LNOT)
      {
        //operand must be a bool constant
        expr = new BoolConstant(!((BoolConstant*) unaryArith->expr)->value);
        return;
      }
      else if(unaryArith->op == BNOT)
      {
        //operand must be an integer
        IntConstant* input = (IntConstant*) unaryArith->expr;
        if(input->isSigned())
          expr = new IntConstant(~(input->sval));
        else
          expr = new IntConstant(~(input->uval));
        return;
      }
      else if(unaryArith->op == SUB)
      {
        //unary sub can be applied to numbers only
        if(auto ic = dynamic_cast<IntConstant*>(unaryArith->expr))
        {
          if(ic->isSigned())
          {
            //conversion is always OK, unless the value is the minimum for the type
            //(because that value in 2s complement has no negation)
            auto intType = (IntegerType*) ic->type;
            if((intType->size == 1 && ic->sval == numeric_limits<int8_t>::min()) ||
               (intType->size == 2 && ic->sval == numeric_limits<int16_t>::min()) ||
               (intType->size == 4 && ic->sval == numeric_limits<int32_t>::min()) ||
               (intType->size == 8 && ic->sval == numeric_limits<int64_t>::min()))
            {
              errMsgLoc(unaryArith, "negating value overflows signed integer");
            }
            //otherwise, always fine to do the conversion
            IntConstant* neg = new IntConstant(-ic->sval);
            neg->type = intType;
            expr = neg;
            return;
          }
          else
          {
            //unsigned values are always considered nonnegative,
            //so negating one is illegal
            errMsgLoc(unaryArith, "can't negate an unsigned value");
          }
        }
        else if(auto fc = dynamic_cast<FloatConstant*>(unaryArith->expr))
        {
          FloatConstant* neg = new FloatConstant;
          neg->fp = -fc->fp;
          neg->dp = -fc->dp;
          neg->type = fc->type;
          expr = neg;
          return;
        }
        else
        {
          INTERNAL_ERROR;
        }
      }
    }
  }
  else if(auto indexed = dynamic_cast<Indexed*>(expr))
  {
    Expression*& grp = indexed->group;
    Expression*& ind = indexed->index;
    foldExpression(grp);
    foldExpression(ind);
    if(grp->constant() && ind->constant())
    {
      if(dynamic_cast<ArrayType*>(grp->type))
      {
        CompoundLiteral* arrValues = (CompoundLiteral*) grp;
        IntConstant* intIndex = (IntConstant*) ind;
        if(intIndex->isSigned())
        {
          if(intIndex->sval < 0 || intIndex->sval >= arrValues->members.size())
          {
            errMsgLoc(indexed, "array index " << intIndex->sval <<
                " out of bounds [0, " << arrValues->members.size() - 1 << ")");
          }
          expr = arrValues->members[intIndex->sval];
          return;
        }
        else
        {
          if(intIndex->uval >= arrValues->members.size())
          {
            errMsgLoc(indexed, "array index " << intIndex->sval <<
                " out of bounds [0, " << arrValues->members.size() - 1 << ")");
          }
          expr = arrValues->members[intIndex->uval];
          return;
        }
      }
      else if(dynamic_cast<MapType*>(grp->type))
      {
        //map lookup returns (T | Error)
        auto mapValue = (MapConstant*) grp;
        auto mapIt = mapValue->values.find(ind);
        if(mapIt == mapValue->values.end())
          expr = new UnionConstant(new ErrorVal, primitives[Prim::ERROR], (UnionType*) indexed->type);
        else
          expr = new UnionConstant(mapIt->second, mapIt->second->type, (UnionType*) indexed->type);
        return;
      }
      INTERNAL_ERROR;
    }
  }
  else if(auto newArray = dynamic_cast<NewArray*>(expr))
  {
    ArrayType* arrType = (ArrayType*) expr->type;
    //create one default instance of an element to find its size in bytes
    auto elemSize = arrType->elem->getDefaultValue()->getConstantSize();
    uint64_t totalElems = 1;
    bool allConstant = true;
    vector<uint64_t> dimVals;
    for(auto& dim : newArray->dims)
    {
      foldExpression(dim);
      if(!dim->constant())
      {
        allConstant = false;
        break;
      }
      auto dimVal = (IntConstant*) dim;
      if(dimVal->isSigned())
      {
        totalElems *= dimVal->sval;
        dimVals.push_back(dimVal->sval);
      }
      else
      {
        totalElems *= dimVal->uval;
        dimVals.push_back(dimVal->uval);
      }
    }
    if(elemSize * totalElems <= maxConstantSize)
    {
      //can create the array
      expr = createArray(dimVals.data(), newArray->dims.size(), arrType->elem);
    }
  }
  else if(auto structMem = dynamic_cast<StructMem*>(expr))
  {
    foldExpression(structMem->base);
    if(structMem->base->constant() && structMem->member.is<Variable*>())
    {
      Variable* var = structMem->member.get<Variable*>();
      //Need to find which member var is, to extract it from compound literal
      auto st = (StructType*) structMem->base->type;
      for(size_t memIndex = 0; memIndex < st->members.size(); memIndex++)
      {
        if(st->members[memIndex] == var)
        {
          expr = ((CompoundLiteral*) structMem->base)->members[memIndex];
        }
      }
      INTERNAL_ERROR;
    }
  }
  else if(auto arrayLen = dynamic_cast<ArrayLength*>(expr))
  {
    foldExpression(arrayLen->array);
    if(arrayLen->array->constant())
    {
      int64_t len = ((CompoundLiteral*) arrayLen->array)->members.size();
      expr = new IntConstant(len);
    }
  }
  else if(auto call = dynamic_cast<CallExpr*>(expr))
  {
    //can try to fold both the callable and each argument
    foldExpression(call->callable);
    for(auto& arg : call->args)
    {
      foldExpression(arg);
    }
  }
  else if(auto isExpr = dynamic_cast<IsExpr*>(expr))
  {
    foldExpression(isExpr->base);
    if(auto uc = dynamic_cast<UnionConstant*>(isExpr->base))
    {
      expr = new BoolConstant(uc->value->type == isExpr->option);
    }
  }
  else if(auto asExpr = dynamic_cast<AsExpr*>(expr))
  {
    foldExpression(asExpr->base);
    if(auto uc = dynamic_cast<UnionConstant*>(asExpr->base))
    {
      if(uc->value->type != asExpr->option)
        errMsgLoc(asExpr, "known at compile time that union value is not a " << asExpr->option->getName());
      expr = uc->value;
    }
  }
  else
  {
    INTERNAL_ERROR;
  }
}

void constantFold(IR::SubroutineIR* subr)
{
  //every expression (including parts of an assignment LHS)
  //may be folded (i.e. myArray[5 + 3] = 8 % 3)
  //
  //recursivly attempt to fold all expressions (input and output) bottom-up
  for(auto& stmt : subr->stmts)
  {
    if(auto assign = dynamic_cast<AssignIR*>(stmt))
    {
      foldExpression(assign->dst);
      foldExpression(assign->src);
    }
    else if(auto call = dynamic_cast<CallIR*>(stmt))
    {
      //foldExpression doesn't evaluate calls,
      //so foldExpression will produce another CallExpr
      Expression* expr = call->eval;
      foldExpression(expr);
      INTERNAL_ASSERT(dynamic_cast<CallExpr*>(expr));
      call->eval = (CallExpr*) expr;
    }
    else if(auto condJump = dynamic_cast<CondJump*>(stmt))
    {
      foldExpression(condJump->cond);
    }
    else if(auto ret = dynamic_cast<ReturnIR*>(stmt))
    {
      if(ret->expr)
        foldExpression(ret->expr);
    }
    else if(auto print = dynamic_cast<PrintIR*>(stmt))
    {
      for(auto& e : print->exprs)
        foldExpression(e);
    }
    else if(auto assertion = dynamic_cast<AssertionIR*>(stmt))
    {
      foldExpression(assertion->asserted);
    }
  }
}

//This holds the constant/nonconstant variable sets for each basic block.
//Populated lazily: assignments to variables add the constant value,
//or "nonconst"

struct ConstantSet
{
  map<Variable*, ConstantVar> vals;
};

bool constantPropagation(SubroutineIR* subr)
{
  bool update = false;
  //Full constant fold/propagate process:
  //  While updates can still be made:
  //  -fold constants
  //  -within BBs (sequentially over statements),
  //   record which variables are constant
  //  -replace usage of constant variables with the constants
  //  -fold constants again
  //  -record which variables have known constant values at exit of BBs
  //  -do constant-prop dataflow analysis
  constantFold(subr);
  for(auto bb : subr->blocks)
  {
    for(int i = bb->start; i < bb->end; i++)
    {
      StatementIR* stmt = subr->stmts[i];
    }
  }
  return update;
}

