#include "ConstantProp.hpp"

using namespace IR;

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
static Expression* convertConstant(Converted* conv)
{
  if(auto intConst = dynamic_cast<IntConstant*>(conv->value))
  {
    //do the conversion which tests for overflow
    return intConst->convert(conv->type);
  }
  else if(auto floatConst = dynamic_cast<FloatConstant*>(conv->value))
  {
    return floatConst->convert(conv->type);
  }
  else if(auto enumConst = dynamic_cast<EnumExpr*>(conv->value))
  {
    return enumConst->convert(conv->type);
  }
  //array/struct/tuple constants can be converted implicitly
  //to each other (all use CompoundLiteral) but individual
  //members (primitives) may need conversion
  else if(auto compLit = dynamic_cast<CompoundLiteral*>(conv->value))
  {
    //attempt to fold all elements (can't proceed unless every
    //one is a constant)
    bool allConstant = true;
    for(auto& mem : compLit->members)
      allConstant = allConstant && foldExpression(mem);
    if(!allConstant)
      return false;
    if(auto st = dynamic_cast<StructType*>(conv->type))
    {
    }
    else if(auto tt = dynamic_cast<TupleType*>(conv->type))
    {
    }
    else if(auto mt = dynamic_cast<MapType*>(conv->type))
    {
      expr = new MapConstant;
      //convert key/value pairs to 
    }
  }
}

//Try to fold an expression, bottom-up
//Can fold all constants in one pass
//
//Return true if any IR changes are made
//Set constant to true if expr is now, or was already, a constant
static bool foldExpression(Expression*& expr)
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
      expr = convertConstant(conv);
      return true;
    }
    else
    {
      return false;
    }
  }
  {
    //already constant and nothing to do
    constant = true;
    return false;
  }
  else if(auto& binArith = dynamic_cast<BinaryArith*&>(expr))
  {
    bool lhsConstant = false;
    bool rhsConstant = false;
    foldExpression(binArith->lhs, lhsConstant);
    foldExpression(binArith->rhs, rhsConstant);
    if(lhsConstant && rhsConstant)
    {
      constant = true;
      return true;
    }
    else
    {
      return false;
    }
  }
  else if(
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

