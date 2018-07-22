#include "ConstantProp.hpp"

using namespace IR;

bool operator==(Value* lhs, Value* rhs);
bool operator<(Value* lhs, Value* rhs);
bool operator>(Value* lhs, Value* rhs)
{
  return rhs < lhs;
}
bool operator<=(Value* lhs, Value* rhs)
{
  return !(lhs > rhs);
}
bool operator<=(Value* lhs, Value* rhs)
{
  return !(lhs > rhs);
}

//Unknown value represents a non-const 
struct UnknownValue : public Value {}

struct Integer : public Value
{
  //note: int8_t is equivalent to char
  variant<uint8_t, int8_t,
          uint16_t, int16_t,
          uint32_t, int32_t,
          uint64_t, int64_t> value;
};

struct Float : public Value
{
  variant<float, double> value;
};

//CompoundValue can be used for arrays, tuples and structs
struct CompoundValue : public Value
{
  vector<Value*> values;
};

struct MapValue : public Value
{
  map<Value*, Value*> value;
};

//For comparisons, always assumy lhs/rhs have exactly the same type
bool operator==(Value* lhs, Value* rhs)
{
#define COMPARE_OPTION(var, type) \
  if(var##LHS->value.is<type>()) \
    return var##LHS->value.get<type>() == var##RHS->value.get<type>();
  //all undefined values are equivalent
  if(dynamic_cast<Undef*>(lhs))
    return true;
  else if(auto intLHS = dynamic_cast<Integer*>(lhs))
  {
    auto intRHS = dynamic_cast<Integer*>(rhs);
    INTERNAL_ASSERT(intRHS);
    //compare LHS and RHS based on the tag
    COMPARE_OPTION(int, uint8_t)
    COMPARE_OPTION(int, int8_t)
    COMPARE_OPTION(int, uint16_t)
    COMPARE_OPTION(int, int16_t)
    COMPARE_OPTION(int, uint32_t)
    COMPARE_OPTION(int, int32_t)
    COMPARE_OPTION(int, uint64_t)
    COMPARE_OPTION(int, int64_t)
    //integers must be one of these types...
    INTERNAL_ERROR;
  }
  else if(auto floatLHS = dynamic_cast<Float*>(lhs))
  {
    auto floatRHS = dynamic_cast<Float*>(rhs);
    INTERNAL_ASSERT(floatRHS);
    COMPARE_OPTION(float, float)
    COMPARE_OPTION(float, double)
    INTERNAL_ERROR;
  }
  else if(auto compoundLHS = dynamic_cast<CompoundValue*>(lhs))
  {
    auto compoundRHS = dynamic_cast<CompoundValue*>(rhs);
    INTERNAL_ASSERT(compoundRHS);
    if(compoundLHS->values.size() != compoundRHS->values.size())
      return false;
    for(size_t i = 0; i < compoundLHS->values.size(); i++)
    {
      if(compoundLHS->values[i] != compoundRHS->values[i])
        return false;
    }
    return true;
  }
  else if(auto mapLHS = dynamic_cast<MapValue*>(lhs))
  {
    auto mapRHS = dynamic_cast<MapValue*>(rhs);
    INTERNAL_ASSERT(mapRHS);
    if(mapLHS->value.size() != mapRHS->value.size())
      return false;
    //look up each key in lhs in rhs,
    //and then make sure value matches
    for(auto kv : mapLHS->value)
    {
      auto iter = mapRHS->value.find(kv.first);
      if(iter == mapRHS->value.end())
        return false;
      if(*iter != kv.second)
        return false;
    }
    return true;
  }
  return false;
}

bool operator<(Value* lhs, Value* rhs)
{
#define COMPARE_OPTION(var, type) \
  if(var##LHS->value.is<type>()) \
    return var##LHS->value.get<type>() < var##RHS->value.get<type>();
  //all undefined values are equivalent, so < is always false
  if(dynamic_cast<Undef*>(lhs))
    return false;
  else if(auto intLHS = dynamic_cast<Integer*>(lhs))
  {
    auto intRHS = dynamic_cast<Integer*>(rhs);
    INTERNAL_ASSERT(intRHS);
    //compare LHS and RHS based on the tag
    COMPARE_OPTION(int, uint8_t)
    COMPARE_OPTION(int, int8_t)
    COMPARE_OPTION(int, uint16_t)
    COMPARE_OPTION(int, int16_t)
    COMPARE_OPTION(int, uint32_t)
    COMPARE_OPTION(int, int32_t)
    COMPARE_OPTION(int, uint64_t)
    COMPARE_OPTION(int, int64_t)
    //integers must be one of those types
    INTERNAL_ERROR;
  }
  else if(auto floatLHS = dynamic_cast<Float*>(lhs))
  {
    auto floatRHS = dynamic_cast<Float*>(rhs);
    INTERNAL_ASSERT(floatRHS);
    COMPARE_OPTION(float, float)
    COMPARE_OPTION(float, double)
    INTERNAL_ERROR;
  }
  else if(auto compoundLHS = dynamic_cast<CompoundValue*>(lhs))
  {
    auto compoundRHS = dynamic_cast<CompoundValue*>(rhs);
    INTERNAL_ASSERT(compoundRHS);
    //do lexicographic comparison
    size_t numCommon = std::min(
        compoundLHS->values.size(), compoundRHS->values.size());
    for(size_t i = 0; i < numCommon; i++)
    {
      if(compoundLHS->values[i] < compoundRHS->values[i])
        return true;
      else if(compoundRHS->values[i] < compoundLHS->values[i])
        return false;
    }
    //common range compared equal, so bigger list is greater.
    return compoundLHS->values.size() < compoundRHS->values.size();
  }
  else if(auto mapLHS = dynamic_cast<MapValue*>(lhs))
  {
    auto mapRHS = dynamic_cast<MapValue*>(rhs);
    INTERNAL_ASSERT(mapRHS);
    //to lexically compare maps, compare keys in sorted order
    auto lhsIter = mapLHS->value.begin();
    auto rhsIter = mapRHS->value.begin();
    while(lhsIter != mapLHS->value.end() &&
        rhsIter != mapRHS->value.end)
    {
      if(lhsIter->first < rhsIter->first)
        return true;
      else if(rhsIter->first < lhsIter->first)
        return false;
      else if(lhsIter->second < rhsIter->second)
        return true;
      else if(rhsIter->second < lhsIter->second)
        return false;
      lhsIter++;
      rhsIter++;
    }
    return lhsIter == mapLHS->value.end();
  }
  INTERNAL_ERROR;
  return false;
}

//Convert an expression to Value, if possible
Value* exprToValue(Expression* e)
{
}

Expression* valueToExpr(Value* v)
{
}

//Try to fold an expression, bottom-up
//Can fold all constants in one pass
//
//Return true if any IR changes are made
//Set constant to true if expr is now, or was already, a constant
static bool foldExpression(Expression*& expr, bool& constant)
{
  bool update = false;
  if(dynamic_cast<IntLiteral*&>(expr) ||
      dynamic_cast<FloatLiteral*&>(expr) ||
      dynamic_cast<StringLiteral*&>(expr) ||
      dynamic_cast<BoolLiteral*&>(expr) ||
      dynamic_cast<CharLiteral*&>(expr) ||
      dynamic_cast<ErrorVal*&>(expr))
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
  return update;
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

