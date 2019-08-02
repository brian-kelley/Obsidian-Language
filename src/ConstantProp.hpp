#ifndef CONSTANT_PROP_H
#define CONSTANT_PROP_H

#include "IR.hpp"

/*

struct CompoundValue;

struct CPValue
{
  //Intersect in with this.
  virtual CPValue* meet(CPValue* in);
  //Given that this describes information known about v,
  //replace occurences of v in e with constants.
  virtual void apply(Variable* v, Expression* e);
};

struct UnknownValue : public CPValue
{
  CPValue* meet(CPValue* other);
  void apply(Variable* v, Expression* e);
};

struct UndefinedValue : public CPValue
{
  CPValue* meet(CPValue* other);
  void apply(Variable* v, Expression* e);
};

//Represents a fixed-size compound value where status of each
//element is merged separately.
//
//Used for arrays (data + dimensions),
//structs/tuples (each member separately),
//and unions (data + tag).
//
//Array data is tracked all-or-nothing so that
//memory usage bounds are easy to enforce. If element 0
//is a constant but element 1 isn't, the whole thing is unknown.
//Constant array values 
struct CompoundValue : public CPValue
{
  CPValue* meet(CPValue* other);
  void apply(Variable* v, Expression* e);
  vector<CPValue*> values;
};

//Represents when the whole value is known as a constant
struct ValueKnown : public CPValue
{
  CPValue* meet(CPValue* other);
  void apply(Variable* v, Expression* e);
  Expression* value;
};

vector<int> getLeading(CompoundLiteral* arr, int ndims);

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
//Uses local constant table
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

bool operator==(const CPValue& lhs, const CPValue& rhs);
inline bool operator!=(const CPValue& lhs, const CPValue& rhs)
{
  return !(lhs == rhs);
}
ostream& operator<<(ostream& os, const CPValue& cv);

//LocalConstantTable efficiently tracks all constants for whole subroutine
struct LocalConstantTable
{
  //Construct initial table, with all variables undefined at every BB
  map<Variable*, int> varTable;
  bool update(Variable* var, CPValue replace);
  bool meetUpdate(int varIndex, int destBlock, CPValue incoming);
  //return current status of the variable
  CPValue getStatus(Variable* var);
  //inner list corresponds to variables
  //outer list corresponds to basic blocks
  vector<map<Variable, CPValue>> constants;
};

//Meet operator for CPValue (for dataflow analysis)
//Is associative/commutative
//c,d constants (c != d), x = nonconstant, ? = undefined
//meet(c, c) = c
//meet(c, d) = x
//meet(x, _) = x
//meet(?, _) = _
CPValue constantMeet(CPValue lhs, CPValue rhs);

*/
#endif

