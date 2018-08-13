#ifndef CONSTANT_PROP_H
#define CONSTANT_PROP_H

#include "IR.hpp"

//Fold global variable initial values
void foldGlobals();
//Find the set of variables modified in each 
void determineModifiedVars();
//Constant folding evaluates as many expressions as possible,
//replacing them with constants
void constantFold(IR::SubroutineIR* subr);
//Constant propagation decides which local variables have
//constant values at each statement, and replaces their
//usage with constants

/* Constant propagation psuedocode:
 *
 *  -Store constant sets at the closing of each BB
 *  -Constant set before first BB is undefined for every var
 *    -good because it lets compiler give error for any usage of undefined var.
 *     (all local var decls have an assignment at the place of declaration,
 *     so they are never undefined after that)
 *  -Insert first BB into a processing queue
 *  -While processing queue is not empty:
 *    -Grab a BB to process
 *    -Join the constant set with those of each incoming BB
 *      (note that join is associative, so join with one BB at a time)
 *    -For each statement:
 *      -Replace VarExprs with constants using constant set (in folding)
 *      -Apply effects of the statement on constant set:
 *       only AssignIR can actually affect local variables,
 *       but also evaluating any member procedure call will modify the struct
 *    -If constant set changed, enqueue all outgoing BBs not already in queue
 */
bool constantPropagation(IR::SubroutineIR* subr);

//Internal
//Try to replace the expression with equivalent constant, if not already constant
//Requires all dependencies to be constant and
//the result to be smaller than maxConstantSize
//Uses both global constant table and local constant table
void foldExpression(Expression*& expr);

struct UndefinedVal {};
struct NonConstant {};

enum ConstPropKind
{
  UNDEFINED_VAL,
  NON_CONSTANT
};

//A constant var can either be "nonconstant" or some constant value
//(undefined values are impossible)
struct ConstantVar
{
  ConstantVar(ConstPropKind cpk)
  {
    switch(cpk)
    {
      case UNDEFINED_VAL:
        val = UndefinedVal();
        break;
      case NON_CONSTANT:
        val = NonConstant();
    }
  }
  ConstantVar(Expression* e)
  {
    val = e;
  }
  variant<UndefinedVal, Nonconstant, Expression*> val;
};

//Meet operator for ConstantVar (for dataflow analysis)
//Is associative/commutative
//c/d = constant, x = nonconstant, ? = undefined
//meet(c, c) = c
//meet(c, d) = x
//meet(x, _) = x
//meet(?, _) = _
ConstantVar constantMeet(ConstantVar& lhs, ConstantVar& rhs);

extern map<Variable*, ConstantVar> globalConstants;
extern map<Subroutine*, set<Variable*>> modifiedVars;

#endif

