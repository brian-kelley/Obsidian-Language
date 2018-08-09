#ifndef CONSTANT_PROP_H
#define CONSTANT_PROP_H

#include "IR.hpp"

//Fold global variable initial values
void foldGlobals();
//Find the set of variables modified in each 
void determineModifiedVars();
//Constant folding evaluates all arithmetic on constants
void constantFold(IR::SubroutineIR* subr);
//Constant propagation decides which local variables have
//constant values at each statement, and replaces their
//usage with constants
//(first just within BBs, then across whole subroutine)
bool constantPropagation(IR::SubroutineIR* subr);

//Internal
void foldExpression(Expression*& expr);

struct Nonconstant {};

//A constant var can either be "nonconstant" or some constant value
//(undefined values are impossible)
struct ConstantVar
{
  ConstantVar()
  {
    val = Nonconstant();
  }
  ConstantVar(Expression* e)
  {
    val = e;
  }
  variant<Nonconstant, Expression*> val;
};

//Join operator for "ConstantVar" (used by dataflow analysis).
//associative/commutative
//
//Since undefined values are impossible, there is no need for a "top" value
ConstantVar join(ConstantVar& lhs, ConstantVar& rhs);

extern map<Variable*, ConstantVar> globalConstants;
extern map<Subroutine*, set<Variable*>> modifiedVars;

#endif

