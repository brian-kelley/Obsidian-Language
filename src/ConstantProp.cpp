#include "ConstantProp.hpp"

using namespace IR;

void constantPropagation(SubroutineIR* subrIR)
{
  //First, go through each BB and replace general Expressions
  //with constants wherever possible (constant folding)
  //
  //Also record which variables hold constant values at exit of BBs
  //
  //Then do constant propagation dataflow analysis across BBs
  //
  //VarExprs of constant variables can then be replaced by constants
}

