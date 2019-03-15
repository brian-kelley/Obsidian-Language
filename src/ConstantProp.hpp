#ifndef CONSTANT_PROP_H
#define CONSTANT_PROP_H

#include "IR.hpp"

//Fold global variable initial values,
//and record which globals have constant initial values AND are never modified
//(these can be replaced by foldExpression)
void findGlobalConstants();

//Constant folding evaluates as many expressions as possible,
//replacing them with constants
//
//DEPRECATED: use constantPropagation instead, which does
//both folding and propagation until no more updates can be done
void constantFold(IR::SubroutineIR* subr);

//constantPropagation determines when variables
//have constant values, and replaces their usage by constants
bool constantPropagation(IR::SubroutineIR* subr);

//Try to replace the expression with equivalent constant, if not already constant
//Requires all dependencies to be constant and
//the result to be smaller than maxConstantSize
//Uses both global constant table and local constant table
bool foldExpression(Expression*& expr, bool isLHS = false);

//apply the effects of a statement to local constant table
bool cpApplyStatement(IR::StatementIR* stmt);
//Apply the effects of an expression to constant table, then fold the expression.
//These steps can't be separated because the constant status of a variable
//can change within an expression
bool cpProcessExpression(Expression*& expr, bool isLHS = false);
//in local constant table, apply the action of "lhs = rhs"
//rhs may or may not be constant, and one or both of lhs/rhs can be compound
bool bindValue(Expression* lhs, Expression* rhs);

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
  ConstantVar()
  {
    val = UndefinedVal();
  }
  ConstantVar(const ConstantVar& other)
  {
    val = other.val;
  }
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
  variant<UndefinedVal, NonConstant, Expression*> val;
};

bool operator==(const ConstantVar& lhs, const ConstantVar& rhs);
inline bool operator!=(const ConstantVar& lhs, const ConstantVar& rhs)
{
  return !(lhs == rhs);
}
ostream& operator<<(ostream& os, const ConstantVar& cv);

//LocalConstantTable efficiently tracks all constants for whole subroutine
struct LocalConstantTable
{
  //Construct initial table, with all variables undefined at every BB
  LocalConstantTable(Subroutine* subr);
  map<Variable*, int> varTable;
  bool update(Variable* var, ConstantVar replace);
  bool update(int varIndex, ConstantVar replace);
  bool meetUpdate(int varIndex, int destBlock, ConstantVar incoming);
  //return current status of the variable
  ConstantVar& getStatus(Variable* var);
  //inner list corresponds to variables
  //outer list corresponds to basic blocks
  vector<vector<ConstantVar>> constants;
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

#endif

