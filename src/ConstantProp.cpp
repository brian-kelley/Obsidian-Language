#include "ConstantProp.hpp"

using namespace IR;

struct Value {};
struct Undef : public Value {};

struct Integer : public Value
{
  variant<uint8_t, int8_t,
          uint16_t, int16_t,
          uint32_t, int32_t,
          uint64_t, int64_t> value;
};

struct Float : public Value
{
  variant<float, double> value;
};

struct CompoundValue : public Value
{
  vector<Value*> values;
};

struct MapValue : public Value
{
  map<Value*, Value*> kv;
};

bool operator==(Value* lhs, Value* rhs)
{
}

bool operator<(Value* lhs, Value* rhs)
{
}

void constantPropagation(SubroutineIR* subr)
{
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
}

