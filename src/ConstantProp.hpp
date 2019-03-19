#ifndef CONSTANT_PROP_H
#define CONSTANT_PROP_H

#include "IR.hpp"

struct ValueKnown;
struct UnionKindKnown;
struct MemberKnown;
struct ArrayLengthKnown;
struct UndefinedValue;
struct NonconstantValue;

//CPValue represents some information
//about a variable's value. This can
//be used to maximize the opportunities
//for constant propagation.
//
//The Dragon Book rules of constant prop still apply:
//if c,d are constants (c != d), x = nonconstant, ? = undefined:
//meet(c, c) = c
//meet(x, _) = x
//meet(?, _) = _
//
//But, if c,d have information in common, meet(c, d) might not be x
struct CPValue
{
  //meet() dynamic-casts other and dispatches
  //to one of the other meetXYZ() functions
  CPValue* meet(CPValue* other);
  //Use information about var's value to expand
  //or simplify expr (return true if changes happen)
  virtual bool apply(Expression*& expr) = 0;
protected:
  //Implementations of different cases for meet().
  //If this has greater information than the type passed in,
  //just calls the commutation: other->meet(this)
  //(this saves on code)
  virtual CPValue* meetValue(ValueKnown* vk) = 0;
  virtual CPValue* meetUnionKind(UnionKindKnown* uk) = 0;
  virtual CPValue* meetMemberKnown(MemberKnown* mk) = 0;
  virtual CPValue* meetArrayLenKnown(ArrayLengthKnown* lk) = 0;
};

struct ValueKnown : public CPValue
{
  ValueKnown(Expression* v) : value(v) {}
  bool apply(Expression*& expr);
  CPValue* meetValue(ValueKnown* vk);
  CPValue* meetUnionKind(UnionKindKnown* uk);
  CPValue* meetMemberKnown(MemberKnown* mk);
  CPValue* meetArrayLenKnown(ArrayLengthKnown* lk);
  Expression* value;
};

//When the current active type of a union is known.
//Can simplify "as", "is"
struct UnionKindKnown : public CPValue
{
  //Constructor for when only a single type is possible.
  UnionKindKnown(UnionType* ut, Type* t);
  //Constructor for general case, and manually mark types possible
  UnionKindKnown(UnionType* ut);
  bool apply(Expression*& expr);
  CPValue* meetValue(ValueKnown* vk);
  CPValue* meetUnionKind(UnionKindKnown* uk);
  CPValue* meetMemberKnown(MemberKnown* mk);
  CPValue* meetArrayLenKnown(ArrayLengthKnown* lk);
  UnionType* ut;
  //Represent the complete set of options which are possible
  //If even one option type T is known to be impossible,
  //"x is T" can be replaced with "false".
  vector<bool> optionsPossible;
};

//For when some specific member(s) of a tuple or struct are known.
struct MemberKnown : public CPValue
{
  bool apply(Expression*& expr);
  vector<CPValue*> members;
};

//When the dimension(s) of an array are known.
//If a multidimensional array, attempts to track all dimensions.
//If e.g. dim 0 is known but dim 1 isn't, then all dims >= 1 are unknown.
//Can replace "x.len", "x[i].len", etc.
struct ArrayLengthKnown : public CPValue
{
  ArrayLengthKnown(vector<size_t>& d) : dimsKnown(d.size()), dims(d) {}
  bool apply(Expression*& expr);
  int dimsKnown;
  vector<size_t> dims;
};

//For uninitialized variables.
//At entry to subroutine, all locals (non parameters)
//are undefined.
struct UndefinedValue : public CPValue
{
  CPValue* meet(CPValue* other);
  bool apply(Expression*& expr);
};

//For variables whose values are completely unknown
struct NonconstantValue : public CVPalue
{
  CPValue* meet(CPValue* other);
  bool apply(Expression*&)
  {
    //Can't do anything
    return false;
  }
};

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

enum CPValueKind
{
  UNDEFINED_VAL,
  NON_CONSTANT
};

//A constant var can either be "nonconstant" or some constant value
//(undefined values are impossible)
struct CPValue
{
  CPValue()
  {
    val = UndefinedVal();
  }
  CPValue(const CPValue& other)
  {
    val = other.val;
  }
  CPValue(CPValueKind kind)
  {
    switch(kind)
    {
      case UNDEFINED_VAL:
        val = UndefinedVal();
        break;
      case NON_CONSTANT:
        val = NonConstant();
    }
  }
  CPValue(Expression* e)
  {
    val = e;
  }
  variant<UndefinedVal, NonConstant, Expression*> val;
};

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

#endif

