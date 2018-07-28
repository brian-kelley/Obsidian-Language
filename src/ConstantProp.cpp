#include "ConstantProp.hpp"

using namespace IR;

//Max number of bytes in constant expressions
const int maxConstantSize = 512;

void determineGlobalConstants()
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
            //so w is not a constant
            IR::globalConstants[w] = false;
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
  if(auto intConst = dynamic_cast<IntConstant*>(value))
  {
    //do the conversion which tests for overflow and enum membership
    return intConst->convert(conv->type);
  }
  else if(auto floatConst = dynamic_cast<FloatConstant*>(value))
  {
    return floatConst->convert(conv->type);
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
      allConstant = allConstant && foldExpression(mem);
    if(!allConstant)
      return false;
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
      for(size_t i = 0; i < compList->members.size(); i++)
      {
        auto kv = dynamic_cast<CompoundLiteral*>(compList->members[i]);
        INTERNAL_ASSERT(kv);
        Expression* key = kv->members[0];
        Expression* value = kv->members[1];
        if(key->type != mt->key)
          key = convertConstant(key, mt->key);
        if(value->type != mt->value)
          value = convertConstant(value, mt->value);
        mc->values[key] = value;
      }
    }
  }
  //????
  INTERNAL_ERROR;
  return nullptr;
}

//Evaluate a numerical binary arithmetic operation.
//Check for integer overflow and invalid operations (e.g. x / 0)
static Expression* evalBinOp(Expression* lhs, int op, Expression* rhs)
{
  auto& lhs = binArith->lhs;
  auto& rhs = binArith->rhs;
  foldExpression(lhs);
  foldExpression(rhs);
  if(!lhs->constant() || !rhs->constant())
    return nullptr;
  //Comparison operations easy because
  //all constant Expressions support comparison
  if(op == CMPEQ)
    return new BoolConstant(lhs == rhs);
  if(op == CMPNEQ)
    return new BoolConstant(!(lhs == rhs));
  if(op == CMPL)
    return new BoolConstant(lhs < rhs);
  if(op == CMPG)
    return new BoolConstant(rhs < lhs);
  if(op == CMPLE)
    return new BoolConstant(!(rhs < lhs));
  if(op == CMPGE)
    return new BoolConstant(!(lhs < rhs));
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
      result->resolveImpl(true);
      return result;
    }
    else if(compoundLHS)
    {
      //array append
      vector<Expression*> resultMembers = compoundLHS->members;
      resultMembers.push_back(rhs);
      CompoundLiteral* result = new CompoundLiteral(resultMembers);
      result->resolveImpl(true);
      return result;
    }
    else if(compoundRHS)
    {
      //array prepend
      vector<Expression*> resultMembers = compoundRHS->members;
      resultMembers.insert(lhs, 0);
      CompoundLiteral* result = new CompoundLiteral(resultMembers);
      result->resolveImpl(true);
      return result;
    }
  }
  //all other binary ops are numerical operations between two ints or two floats
  FloatConstant* lhsFloat = dynamic_cast<FloatConstant*>(lhs);
  FloatConstant* rhsFloat = dynamic_cast<FloatConstant*>(rhs);
  IntConstant* lhsInt = dynamic_cast<IntConstant*>(lhs);
  IntConstant* rhsInt = dynamic_cast<IntConstant*>(rhs);
  bool useFloat = lhsFloat || rhsFloat;
  bool useInt = lhsInt || rhsInt;
  INTERNAL_ASSERT(useFloat ^ useInt);
  if(useFloat)
    return lhsFloat->binOp(op, rhsFloat);
  return lhsInt->binOp(op, rhsInt);
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
static void foldExpression(Expression*& expr)
{
  if(dynamic_cast<IntConstant*>(expr) ||
      dynamic_cast<FloatConstant*>(expr) ||
      dynamic_cast<StringConstant*>(expr) ||
      dynamic_cast<EnumExpr*>(expr) ||
      dynamic_cast<BoolConstant*>(expr) ||
      dynamic_cast<MapConstant*>(expr) ||
      dynamic_cast<ErrorVal*>(expr))
  {
    //already completely folded constant, nothing to do
    return true;
  }
  else if(auto conv = dynamic_cast<Converted*>(expr))
  {
    foldExpression(conv->value);
    if(conv->value->constant())
    {
      expr = convertConstant(conv->value, conv->type);
      return true;
    }
    else
    {
      return false;
    }
  }
  else if(auto binArith = dynamic_cast<BinaryArith*>(expr))
  {
    //evalBinOp returns null if expr not constant or is too big to evaluate
    Expression* result = evalBinOp(binArith->lhs, binArith->op, binArith->rhs);
    if(result)
      expr = result;
    return;
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
        if(((IntegerType*) input->type)->isSigned)
          expr = new IntConstant(~(input->sval));
        else
          expr = new IntConstant(~(input->uval));
        return;
      }
    }
    return;
  }
}

bool constantFold(IR::SubroutineIR* subr)
{
  //every expression (including parts of an assignment LHS)
  //may be folded (i.e. myArray[5 + 3] = 8 % 3)
  //
  //recursivly attempt to fold expressions bottom-up,
  //remembering which input expressions are constants
}

bool constantPropagation(SubroutineIR* subr)
{
  bool update = false;
  //First, go through each BB and replace general Expressions
  //with constants wherever possible (constant folding)
  //
  //Also record which variables hold constant values at exit of BBs
  //
  //Then do constant propagation dataflow analysis across BBs
  //
  //VarExprs of constant variables can then be replaced by constant
  for(auto bb : subr->blocks)
  {
    for(int i = bb->start; i < bb->end; i++)
    {
      Statement* stmt = subr->stmts[i];
    }
  }
  return update;
}

