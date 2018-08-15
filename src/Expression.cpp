#include "Expression.hpp"
#include "Variable.hpp"
#include "Scope.hpp"
#include "Subroutine.hpp"
#include <limits>

using std::numeric_limits;

/**************
 * UnaryArith *
 **************/

UnaryArith::UnaryArith(int o, Expression* e)
  : op(o), expr(e) {}

void UnaryArith::resolveImpl()
{
  resolveExpr(expr);
  if(op == LNOT && expr->type != primitives[Prim::BOOL])
  {
    errMsgLoc(this, "! operand must be a bool");
  }
  else if(op == BNOT && !expr->type->isInteger())
  {
    errMsgLoc(this, "~ operand must be an integer");
  }
  else if(op == SUB && !expr->type->isNumber())
  {
    errMsgLoc(this, "unary - operand must be a number");
  }
  type = expr->type;
  resolved = true;
}

set<Variable*> UnaryArith::getReads()
{
  return expr->getReads();
}

bool operator==(const UnaryArith& lhs, const UnaryArith& rhs)
{
  return lhs.op == rhs.op && *lhs.expr == *rhs.expr;
}

bool operator<(const UnaryArith& lhs, const UnaryArith& rhs)
{
  if(lhs.op < rhs.op)
    return true;
  else if(lhs.op > rhs.op)
    return false;
  else if(*lhs.expr < *rhs.expr)
    return true;
  return false;
}

/***************
 * BinaryArith *
 ***************/

BinaryArith::BinaryArith(Expression* l, int o, Expression* r) : op(o), lhs(l), rhs(r) {}

void BinaryArith::resolveImpl()
{
  resolveExpr(lhs);
  resolveExpr(rhs);
  //Type check the operation
  auto ltype = lhs->type;
  auto rtype = rhs->type;
  switch(op)
  {
    case LOR:
    case LAND:
    {
      if(ltype != primitives[Prim::BOOL] ||
         rtype != primitives[Prim::BOOL])
      {
        errMsgLoc(this, "operands to " << operatorTable[op] << " must be bools.");
      }
      //type of expression is always bool
      this->type = primitives[Prim::BOOL];
      break;
    }
    case BOR:
    case BAND:
    case BXOR:
    {
      //both operands must be integers
      if(!(ltype->isInteger()) || !(rtype->isInteger()))
      {
        errMsgLoc(this, "operands to " << operatorTable[op] << " must be integers.");
      }
      //the resulting type is the wider of the two integers, favoring unsigned
      type = promote(ltype, rtype);
      if(ltype != type)
      {
        lhs = new Converted(lhs, type);
      }
      if(rtype != type)
      {
        rhs = new Converted(rhs, type);
      }
      break;
    }
    case PLUS:
    {
      //intercept plus operator for arrays (concatenation, prepend, append)
      auto lhsAT = dynamic_cast<ArrayType*>(ltype);
      auto rhsAT = dynamic_cast<ArrayType*>(rtype);
      if(lhsAT && rhsAT)
      {
        if(rhsAT->canConvert(lhsAT))
        {
          type = ltype;
        }
        else if(lhsAT->canConvert(rhsAT))
        {
          type = rtype;
        }
        else
        {
          errMsgLoc(this, "incompatible array concatenation operands: " << ltype->getName() << " and " << rtype->getName());
        }
        if(ltype != type)
        {
          lhs = new Converted(lhs, type);
        }
        if(rtype != type)
        {
          rhs = new Converted(rhs, type);
        }
        break;
      }
      else if(lhsAT)
      {
        //array append
        Type* subtype = lhsAT->subtype;
        if(!subtype->canConvert(rtype))
        {
          errMsgLoc(this, "can't append type " << rtype->getName() << " to " << ltype->getName());
        }
        type = ltype;
        if(subtype != rtype)
        {
          rhs = new Converted(rhs, subtype);
        }
        break;
      }
      else if(rhsAT)
      {
        //array prepend
        Type* subtype = rhsAT->subtype;
        if(!subtype->canConvert(rtype))
        {
          errMsgLoc(this, "can't prepend type " << ltype->getName() << " to " << rtype->getName());
        }
        type = rtype;
        if(subtype != ltype)
        {
          lhs = new Converted(lhs, subtype);
        }
        break;
      }
    }
    case SUB:
    case MUL:
    case DIV:
    case MOD:
    {
      //TODO (CTE): error for div/mod with rhs = 0
      //TODO: support array concatenation with +
      if(!(ltype->isNumber()) || !(rtype->isNumber()))
      {
        errMsgLoc(this, "operands to arithmetic operators must be numbers.");
      }
      type = promote(ltype, rtype);
      if(ltype != type)
      {
        lhs = new Converted(lhs, type);
      }
      if(rtype != type)
      {
        rhs = new Converted(rhs, type);
      }
      break;
    }
    case SHL:
    case SHR:
    {
      //TODO (CTE): error for rhs < 0
      if(!(ltype->isInteger()) || !(rtype->isInteger()))
      {
        errMsgLoc(this, "operands to " << operatorTable[op] << " must be integers.");
      }
      type = ltype;
      break;
    }
    case CMPEQ:
    case CMPNEQ:
    case CMPL:
    case CMPLE:
    case CMPG:
    case CMPGE:
    {
      //To determine if comparison is allowed, lhs or rhs needs to be convertible to the type of the other
      //here, use the canConvert that takes an expression
      type = primitives[Prim::BOOL];
      if(!ltype->canConvert(rtype) && !rtype->canConvert(ltype))
      {
        errMsgLoc(this, ltype->getName() <<
            " and " << rtype->getName() << " can't be compared.");
      }
      if(ltype != rtype)
      {
        if(ltype->canConvert(rtype))
        {
          rhs = new Converted(rhs, ltype);
        }
        else
        {
          lhs = new Converted(lhs, rtype);
        }
      }
      break;
    }
    default: INTERNAL_ERROR;
  }
  resolved = true;
}

set<Variable*> BinaryArith::getReads()
{
  auto reads = lhs->getReads();
  auto rhsReads = rhs->getReads();
  reads.insert(rhsReads.begin(), rhsReads.end());
  return reads;
}

bool operator==(const BinaryArith& lhs, const BinaryArith& rhs)
{
  if(lhs.op != rhs.op)
    return false;
  if(*lhs.lhs != *rhs.lhs)
    return false;
  if(*lhs.rhs != *rhs.rhs)
    return false;
  return true;
}

bool operator<(const BinaryArith& lhs, const BinaryArith& rhs)
{
  if(lhs.op < rhs.op)
    return true;
  else if(lhs.op > rhs.op)
    return false;
  else if(*lhs.lhs < *rhs.lhs)
    return true;
  else if(*lhs.lhs > *rhs.lhs)
    return false;
  else if(*lhs.rhs < *rhs.rhs)
    return true;
  return false;
}

/**********************
 * Primitive Literals *
 **********************/

Expression* IntConstant::convert(Type* t)
{
  if(auto dstType = dynamic_cast<IntegerType*>(t))
  {
    //just give this constant the same value,
    //then make sure the value fits
    IntConstant* intConstant = new IntConstant;
    if(isSigned() == dstType->isSigned)
    {
      intConstant->uval = uval;
      intConstant->sval = sval;
    }
    else if(isSigned())
    {
      //this is signed, so make sure value isn't negative
      if(sval < 0)
      {
        errMsgLoc(this, "cannot convert negative value to unsigned");
      }
      intConstant->uval = sval;
    }
    else if(dstType->isSigned)
    {
      if(uval > numeric_limits<int64_t>::max())
      {
        errMsgLoc(this, "unsigned value too big to convert to any signed type");
      }
      intConstant->sval = uval;
    }
    intConstant->type = dstType;
    if(!intConstant->checkValueFits())
    {
      if(isSigned())
      {
        errMsgLoc(this, "value " << sval <<
            " does not fit in " << dstType->getName());
      }
      else
      {
        errMsgLoc(this, "value " << uval
            << " does not fit in " << dstType->getName());
      }
    }
    return intConstant;
  }
  else if(auto enumType = dynamic_cast<EnumType*>(t))
  {
    //when converting int to enum,
    //make sure value is actually in enum
    for(auto ec : enumType->values)
    {
      if(isSigned())
      {
        if(ec->fitsS64 && ec->sval == sval)
          return new EnumExpr(ec);
      }
      else
      {
        if(ec->fitsU64 && ec->uval == uval)
          return new EnumExpr(ec);
      }
    }
    if(isSigned())
    {
      errMsgLoc(this, "value " << sval <<
          " is not in enum " << enumType->name);
    }
    else
    {
      errMsgLoc(this, "value " << uval <<
          " is not in enum " << enumType->name);
    }
    return nullptr;
  }
  else if(auto floatType = dynamic_cast<FloatType*>(t))
  {
    //integer -> float/double conversion always succeeds
    if(floatType->size == 4)
    {
      if(isSigned())
        return new FloatConstant((float) sval);
      else
        return new FloatConstant((float) uval);
    }
    else
    {
      if(isSigned())
        return new FloatConstant((double) sval);
      else
        return new FloatConstant((double) uval);
    }
  }
  else if(dynamic_cast<CharType*>(t))
  {
    auto charInt = (IntegerType*) primitives[Prim::UBYTE];
    if(isSigned())
    {
      if(sval >= 0 && sval <= charInt->maxUnsignedVal())
        return new CharConstant((char) sval);
    }
    else
    {
      if(uval <= charInt->maxUnsignedVal())
        return new CharConstant((char) uval);
    }
    errMsgLoc(this, "integer value doesn't fit in char");
  }
  INTERNAL_ERROR;
  return nullptr;
}

bool IntConstant::checkValueFits()
{
  auto intType = (IntegerType*) type;
  int size = intType->size;
  if(intType->isSigned)
  {
    switch(size)
    {
      case 1:
        return numeric_limits<int8_t>::min() <= sval &&
          sval <= numeric_limits<int8_t>::max();
      case 2:
        return numeric_limits<int16_t>::min() <= sval &&
          sval <= numeric_limits<int16_t>::max();
      case 4:
        return numeric_limits<int32_t>::min() <= sval &&
          sval <= numeric_limits<int32_t>::max();
      default:
        return true;
    }
  }
  else
  {
    switch(size)
    {
      case 1:
        return uval <= numeric_limits<uint8_t>::max();
      case 2:
        return uval <= numeric_limits<uint16_t>::max();
      case 4:
        return uval <= numeric_limits<uint32_t>::max();
      default:
        return true;
    }
  }
  return false;
}

IntConstant* IntConstant::binOp(int op, IntConstant* rhs)
{
  //most operations produce an expression of same type
  IntConstant* result = nullptr;
  if(isSigned())
  {
    result = new IntConstant((int64_t) 0);
  }
  else
  {
    result = new IntConstant((uint64_t) 0);
  }
  result->type = type;
  //set the type (later check that result actually fits)
#define DO_OP(name, op) \
  case name: \
    if(isSigned()) \
      result->sval = sval op rhs->sval; \
    else \
      result->uval  = uval op rhs->uval; \
    break;
  switch(op)
  {
    DO_OP(PLUS, +)
    DO_OP(SUB, -)
    DO_OP(MUL, *)
    case DIV:
    case MOD:
    {
      //div/mod need extra logic to check for div-by-0
      //important to avoid exception in compiler!
      if((isSigned() && rhs->sval == 0) ||
          (!isSigned() && rhs->uval == 0))
      {
        errMsgLoc(this, (op == DIV ? "div" : "mod") << " by 0");
      }
      if(op == DIV)
      {
        if(isSigned())
          result = new IntConstant(sval / rhs->sval);
        else
          result = new IntConstant(uval / rhs->uval);
      }
      else
      {
        if(isSigned())
          result = new IntConstant(sval % rhs->sval);
        else
          result = new IntConstant(uval % rhs->uval);
      }
      break;
    }
    DO_OP(BOR, |)
    DO_OP(BXOR, ^)
    DO_OP(BAND, &)
    DO_OP(SHL, <<)
    DO_OP(SHR, >>)
    default:
    INTERNAL_ERROR;
  }
  if(!result->checkValueFits())
  {
    errMsgLoc(this, "operation overflows " << type->getName());
  }
  INTERNAL_ASSERT(this->isSigned() == result->isSigned());
  return result;
#undef DO_OP
}

bool operator==(const IntConstant& lhs, const IntConstant& rhs)
{
  IntegerType* lhsType = (IntegerType*) lhs.type;
  IntegerType* rhsType = (IntegerType*) rhs.type;
  if(lhsType != rhsType)
    return false;
  if(lhs.isSigned())
    return lhs.sval == rhs.sval;
  return lhs.uval == rhs.uval;
}

bool operator<(const IntConstant& lhs, const IntConstant& rhs)
{
  //signed < unsigned, then narrower < wider, then compare values
  if(lhs.isSigned() && !rhs.isSigned())
    return true;
  else if(!lhs.isSigned() && rhs.isSigned())
    return false;
  IntegerType* lhsType = (IntegerType*) lhs.type;
  IntegerType* rhsType = (IntegerType*) rhs.type;
  if(lhsType->size < rhsType->size)
    return true;
  else if(lhsType->size > rhsType->size)
    return false;
  cout << "Comparing two IntConstants with same type.\n";
  if(lhs.isSigned())
  {
    cout << "Signed, so comparing " << lhs.sval << " with " << rhs.sval << "\n";
    return lhs.sval < rhs.sval;
  }
  cout << "Unsigned, so comparing " << lhs.uval << " with " << rhs.uval << "\n";
  return lhs.uval < rhs.uval;
}

Expression* FloatConstant::convert(Type* t)
{
  //first, just promote to double
  double val = type == primitives[Prim::FLOAT] ? fp : dp;
  if(auto intType = dynamic_cast<IntegerType*>(t))
  {
    //make sure val fits in a 64-bit integer,
    //then make a 64-bit version of value and narrow it to desired type
    if(intType->isSigned)
    {
      if(val < numeric_limits<int64_t>::min() ||
          val > numeric_limits<int64_t>::max())
      {
        errMsgLoc(this, "floating-point value " << val <<
            " can't be represented in any signed integer");
      }
      IntConstant asLong((int64_t) val);
      return asLong.convert(t);
    }
    else
    {
      if(val < 0 || val > numeric_limits<uint64_t>::max())
      {
        errMsgLoc(this, "floating-point value " << val <<
            " can't be represented in any unsigned integer");
      }
      IntConstant asULong((uint64_t) val);
      return asULong.convert(t);
    }
  }
  else if(auto floatType = dynamic_cast<FloatType*>(t))
  {
    if(floatType->size == 4)
    {
      return new FloatConstant((float) val);
    }
    else
    {
      return new FloatConstant(val);
    }
  }
  else if(dynamic_cast<EnumType*>(t))
  {
    //temporarily make an integer value, then convert that to enum
    if(val < 0)
    {
      IntConstant* asLong = (IntConstant*) convert(primitives[Prim::LONG]);
      return asLong->convert(t);
    }
    else
    {
      IntConstant* asULong = (IntConstant*) convert(primitives[Prim::ULONG]);
      return asULong->convert(t);
    }
  }
  INTERNAL_ERROR;
  return nullptr;
}

FloatConstant* FloatConstant::binOp(int op, FloatConstant* rhs)
{
  //most operations produce an expression of same type
  FloatConstant* result = new FloatConstant(0.0);
  switch(op)
  {
    case PLUS:
      if(isDoublePrec())
        result->dp = dp + rhs->dp;
      else
        result->fp = fp + rhs->fp;
      break;
    case SUB:
      if(isDoublePrec())
        result->dp = dp - rhs->dp;
      else
        result->fp = fp - rhs->fp;
      break;
    case MUL:
      if(isDoublePrec())
        result->dp = dp * rhs->dp;
      else
        result->fp = fp * rhs->fp;
      break;
    case DIV:
    {
      //div/mod need extra logic to check for div-by-0
      //important to avoid exception in compiler!
      if((isDoublePrec() && rhs->dp == 0) || (!isDoublePrec() && rhs->fp == 0))
      {
        errMsgLoc(this, "divide by 0");
      }
      if(isDoublePrec())
        result->dp = dp / rhs->dp;
      else
        result->fp = fp / rhs->fp;
      break;
    }
    default:
    INTERNAL_ERROR;
  }
  //set the type and then check that result actually fits
  result->type = type;
  return result;
}

bool operator==(const FloatConstant& lhs, const FloatConstant& rhs)
{
  if(lhs.type != rhs.type)
    return false;
  if(lhs.isDoublePrec())
    return lhs.dp == rhs.dp;
  return lhs.fp == rhs.fp;
}

bool operator<(const FloatConstant& lhs, const FloatConstant& rhs)
{
  //float < double, then compare values
  if(!lhs.isDoublePrec() && rhs.isDoublePrec())
    return true;
  if(lhs.isDoublePrec() && !rhs.isDoublePrec())
    return false;
  if(lhs.isDoublePrec())
    return lhs.dp < rhs.dp;
  return lhs.fp < rhs.fp;
}

bool operator==(const StringConstant& lhs, const StringConstant& rhs)
{
  return lhs.value == rhs.value;
}

bool operator<(const StringConstant& lhs, const StringConstant& rhs)
{
  return lhs.value < rhs.value;
}

bool operator==(const CharConstant& lhs, const CharConstant& rhs)
{
  return lhs.value == rhs.value;
}

bool operator<(const CharConstant& lhs, const CharConstant& rhs)
{
  return lhs.value < rhs.value;
}

bool operator==(const BoolConstant& lhs, const BoolConstant& rhs)
{
  return lhs.value == rhs.value;
}

bool operator<(const BoolConstant& lhs, const BoolConstant& rhs)
{
  return !lhs.value && rhs.value;
}

bool ExprCompare::operator()(const Expression* lhs, const Expression* rhs)
{
  return *lhs < *rhs;
}

bool operator==(const MapConstant& lhs, const MapConstant& rhs)
{
  auto& l = lhs.values;
  auto& r = rhs.values;
  if(l.size() != r.size())
    return false;
  auto lhsIt = l.begin();
  auto rhsIt = r.begin();
  while(lhsIt != l.end())
  {
    if(lhsIt->first != rhsIt->first ||
        lhsIt->second != rhsIt->second)
    {
      return false;
    }
    lhsIt++;
    rhsIt++;
  }
  return true;
}

bool operator<(const MapConstant& lhs, const MapConstant& rhs)
{
  auto& l = lhs.values;
  auto& r = rhs.values;
  auto lhsIt = l.begin();
  auto rhsIt = r.begin();
  while(lhsIt != l.end() &&
      rhsIt != r.end())
  {
    if(lhsIt->first < rhsIt->first)
      return true;
    else if(lhsIt->first > rhsIt->first)
      return false;
    else if(lhsIt->second < rhsIt->second)
      return true;
    else if(lhsIt->second> rhsIt->second)
      return false;
    lhsIt++;
    rhsIt++;
  }
  if(lhsIt == l.end())
  {
    //lhs is a smaller set
    return true;
  }
  return false;
}

bool operator==(const UnionConstant& lhs, const UnionConstant& rhs)
{
  return lhs.option == rhs.option && lhs.value == rhs.value;
}

bool operator<(const UnionConstant& lhs, const UnionConstant& rhs)
{
  if(lhs.option < rhs.option)
    return true;
  else if(lhs.option > rhs.option)
    return false;
  else if(lhs.value < rhs.value)
    return true;
  return false;
}

/*******************
 * CompoundLiteral *
 *******************/

CompoundLiteral::CompoundLiteral(vector<Expression*>& mems)
  : members(mems) {}

void CompoundLiteral::resolveImpl()
{
  //first, try to resolve all members
  bool allResolved = true;
  lvalue = true;
  for(size_t i = 0; i < members.size(); i++)
  {
    resolveExpr(members[i]);
    if(!members[i]->assignable())
    {
      lvalue = false;
    }
  }
  if(!allResolved)
    return;
  vector<Type*> memberTypes;
  for(auto mem : members)
  {
    memberTypes.push_back(mem->type);
  }
  type = getTupleType(memberTypes);
  resolved = true;
}

set<Variable*> CompoundLiteral::getReads()
{
  set<Variable*> reads;
  for(auto mem : members)
  {
    auto memReads = mem->getReads();
    reads.insert(memReads.begin(), memReads.end());
  }
  return reads;
}

set<Variable*> CompoundLiteral::getWrites()
{
  set<Variable*> writes;
  for(auto mem : members)
  {
    auto memWrites = mem->getWrites();
    writes.insert(memWrites.begin(), memWrites.end());
  }
  return writes;
}

bool operator==(const CompoundLiteral& lhs, const CompoundLiteral& rhs)
{
  auto& l = lhs.members;
  auto& r = rhs.members;
  if(l.size() != r.size())
    return false;
  for(size_t i = 0; i < l.size(); i++)
  {
    if(l[i] != r[i])
      return false;
  }
  return true;
}

bool operator<(const CompoundLiteral& lhs, const CompoundLiteral& rhs)
{
  auto& l = lhs.members;
  auto& r = rhs.members;
  size_t i;
  for(i = 0;; i++)
  {
    if(i == l.size() || i == r.size())
      break;
    if(l[i] < r[i])
      return true;
    else if(l[i] > r[i])
      return false;
  }
  return i == l.size() && i != r.size();
}

/***********
 * Indexed *
 ***********/

Indexed::Indexed(Expression* grp, Expression* ind)
  : group(grp), index(ind) {}

void Indexed::resolveImpl()
{
  resolveExpr(group);
  resolveExpr(index);
  //Indexing a Tuple (literal, variable or call) requires the index to be an IntLit
  //Anything else is assumed to be an array and then the index can be any integer expression
  if(dynamic_cast<CompoundLiteral*>(group))
  {
    errMsgLoc(this, "Can't index a compound literal - assign it to an array first.");
  }
  //note: ok if this is null
  //in all other cases, group must have a type now
  if(auto tt = dynamic_cast<TupleType*>(group->type))
  {
    //group's type is a Tuple, whether group is a literal, var or call
    //make sure the index is an IntLit
    auto intIndex = dynamic_cast<IntConstant*>(index);
    if(intIndex)
    {
      uint64_t idx = intIndex->uval;
      bool outOfBounds = false;
      if(intIndex->isSigned())
      {
        if(intIndex->sval < 0)
          outOfBounds = true;
        else
          idx = intIndex->sval;
      }
      if(idx>= tt->members.size())
        outOfBounds = true;
      if(outOfBounds)
        errMsgLoc(this, "tuple subscript out of bounds");
    }
    else
    {
      errMsgLoc(this, "tuple subscript must be an integer constant.");
    }
  }
  else if(auto at = dynamic_cast<ArrayType*>(group->type))
  {
    //group must be an array
    type = at->subtype;
  }
  else if(auto mt = dynamic_cast<MapType*>(group->type))
  {
    //make sure ind can be converted to the key type
    if(!mt->key->canConvert(index->type))
    {
      errMsgLoc(this, "used incorrect type to index map");
    }
    //map lookup can fail, so return a "maybe" of value
    type = maybe(mt->value);
  }
  else
  {
    errMsgLoc(this, "expression can't be subscripted (is not an array, tuple or map)");
  }
  resolved = true;
}

set<Variable*> Indexed::getReads()
{
  auto reads = group->getReads();
  auto indexReads = index->getReads();
  reads.insert(indexReads.begin(), indexReads.end());
  return reads;
}

set<Variable*> Indexed::getWrites()
{
  return group->getWrites();
}

bool operator==(const Indexed& lhs, const Indexed& rhs)
{
  return lhs.group == rhs.group && lhs.index == rhs.index;
}

bool operator<(const Indexed& lhs, const Indexed& rhs)
{
  return lhs.group < rhs.group ||
    (lhs.group == rhs.group && lhs.index < rhs.index);
}

/************
 * CallExpr *
 ************/

CallExpr::CallExpr(Expression* c, vector<Expression*>& a)
{
  auto ct = dynamic_cast<CallableType*>(c->type);
  if(!ct)
  {
    errMsg("expression is not callable");
  }
  callable = c;
  args = a;
}

void CallExpr::resolveImpl()
{
  resolveExpr(callable);
  auto callableType = dynamic_cast<CallableType*>(callable->type);
  if(!callableType)
  {
    errMsgLoc(this, "attempt to call non-callable expression");
  }
  type = callableType->returnType;
  for(size_t i = 0; i < args.size(); i++)
  {
    resolveExpr(args[i]);
  }
  //make sure number of arguments matches
  if(callableType->argTypes.size() != args.size())
  {
    errMsgLoc(this, "in call to " <<
        (callableType->ownerStruct ? "" : "static") <<
        (callableType->pure ? "function" : "procedure") <<
        ", expected " <<
        callableType->argTypes.size() <<
        " arguments but " <<
        args.size() << " were provided");
  }
  for(size_t i = 0; i < args.size(); i++)
  {
    //make sure arg value can be converted to expected type
    if(!callableType->argTypes[i]->canConvert(args[i]->type))
    {
      errMsg("argument " << i + 1 << " to " << (callableType->ownerStruct ? "" : "static") <<
        (callableType->pure ? "function" : "procedure") << " has wrong type (expected " <<
        callableType->argTypes[i]->getName() << " but got " <<
        (args[i]->type ? args[i]->type->getName() : "incompatible compound literal") << ")");
    }
    if(callableType->argTypes[i] != args[i]->type)
    {
      args[i] = new Converted(args[i], callableType->argTypes[i]);
    }
  }
  resolved = true;
}

set<Variable*> CallExpr::getReads()
{
  auto reads = callable->getReads();
  for(auto arg : args)
  {
    auto argReads = arg->getReads();
    reads.insert(argReads.begin(), argReads.end());
  }
  return reads;
}

bool operator==(const CallExpr& lhs, const CallExpr& rhs)
{
  if(lhs.callable != rhs.callable)
    return false;
  INTERNAL_ASSERT(lhs.args.size() == rhs.args.size());
  for(size_t i = 0; i < lhs.args.size(); i++)
  {
    if(lhs.args[i] != rhs.args[i])
      return false;
  }
  return true;
}

bool operator<(const CallExpr& lhs, const CallExpr& rhs)
{
  if(lhs.callable < rhs.callable)
    return true;
  else if(lhs.callable > rhs.callable)
    return false;
  //otherwise the callable is identical, so both have same # arguments
  INTERNAL_ASSERT(lhs.args.size() == rhs.args.size());
  for(size_t i = 0; i < lhs.args.size(); i++)
  {
    if(lhs.args[i] < rhs.args[i])
      return true;
    else if(lhs.args[i] > rhs.args[i])
      return false;
  }
  return false;
}

/***********
 * VarExpr *
 ***********/

VarExpr::VarExpr(Variable* v, Scope* s) : var(v), scope(s) {}
VarExpr::VarExpr(Variable* v) : var(v), scope(nullptr) {}

void VarExpr::resolveImpl()
{
  var->resolve();
  type = var->type;
  //scope is only provided for user-written VarExprs,
  //which need to be checked here for function correctness
  //(can't access any variables outside func scope)
  if(scope)
  {
    //only thing to check here is that var lives within
    //the innermost function containing the variable's usage
    //(or, if innermost function is a member, a member of that struct)
    Scope* varScope = var->scope;
    Scope* fnScope = scope->getFunctionContext();
    if(!fnScope->contains(varScope))
    {
      errMsgLoc(this, "use of variable " << var->name << " here violates function purity");
    }
  }
  resolved = true;
}

set<Variable*> VarExpr::getReads()
{
  set<Variable*> reads;
  reads.insert(var);
  return reads;
}

set<Variable*> VarExpr::getWrites()
{
  set<Variable*> writes;
  writes.insert(var);
  return writes;
}

bool operator==(const VarExpr& lhs, const VarExpr& rhs)
{
  return lhs.var == rhs.var;
}

bool operator<(const VarExpr& lhs, const VarExpr& rhs)
{
  return lhs.var->id < rhs.var->id;
}

/******************
 * SubroutineExpr *
 ******************/

SubroutineExpr::SubroutineExpr(Subroutine* s, Scope* scope)
{
  thisObject = nullptr;
  subr = s;
  exSubr = nullptr;
  usage = scope;
}

SubroutineExpr::SubroutineExpr(Expression* root, Subroutine* s)
{
  thisObject = root;
  subr = s;
  exSubr = nullptr;
  usage = nullptr;
}

SubroutineExpr::SubroutineExpr(ExternalSubroutine* es)
{
  thisObject = nullptr;
  subr = nullptr;
  exSubr = es;
  usage = nullptr;
}

void SubroutineExpr::resolveImpl()
{
  if(subr)
    type = subr->type;
  else if(exSubr)
    type = exSubr->type;
  else
    INTERNAL_ERROR;
  if(!thisObject && subr && subr->type->ownerStruct)
  {
    errMsgLoc(this, \
        "can't call member subroutine " << \
        subr->type->ownerStruct->name << '.' \
        << subr->name << \
        "\nwithout providing 'this' object");
  }
  else if(thisObject &&
      ((subr && !subr->type->ownerStruct) || exSubr))
  {
    errMsgLoc(this, \
        "can't call non-member subroutine " << \
        (subr ? subr->name : exSubr->name) << \
        " on an object");
  }
}

bool operator==(const SubroutineExpr& lhs, const SubroutineExpr& rhs)
{
  return lhs.subr == rhs.subr &&
    lhs.exSubr == rhs.exSubr &&
    lhs.thisObject == rhs.thisObject;
}

bool operator<(const SubroutineExpr& lhs, const SubroutineExpr& rhs)
{
  enum
  {
    NORMAL,
    MEMBER,
    EXTERNAL
  };
  int lhsKind = NORMAL;
  if(lhs.thisObject)
    lhsKind = MEMBER;
  else if(lhs.exSubr)
    lhsKind = EXTERNAL;
  int rhsKind = NORMAL;
  if(rhs.thisObject)
    rhsKind = MEMBER;
  else if(rhs.exSubr)
    rhsKind = EXTERNAL;
  if(lhsKind < rhsKind)
    return true;
  else if(lhsKind > rhsKind)
    return false;
  if(lhsKind == NORMAL)
    return lhs.subr->id < rhs.subr->id;
  else if(lhsKind == MEMBER)
  {
    if(lhs.subr->id < rhs.subr->id)
      return true;
    else if(lhs.subr->id > rhs.subr->id)
      return false;
    else if(lhs.thisObject < rhs.thisObject)
      return true;
    else if(lhs.thisObject > rhs.thisObject)
      return false;
    return false;
  }
  else
    return lhs.exSubr->id < rhs.exSubr->id;
}

/*************
 * StructMem *
 *************/

StructMem::StructMem(Expression* b, Variable* v)
{
  base = b;
  member = v;
}

StructMem::StructMem(Expression* b, Subroutine* s)
{
  base = b;
  member = s;
}

void StructMem::resolveImpl()
{
  resolveExpr(base);
  //make sure that member is actually a member of base
  if(member.is<Variable*>())
  {
    auto var = member.get<Variable*>();
    type = var->type;
    if(var->owner != base->type)
    {
      INTERNAL_ERROR;
    }
  }
  else
  {
    auto subr = member.get<Subroutine*>();
    type = subr->type;
    if(subr->type->ownerStruct != base->type)
    {
      INTERNAL_ERROR;
    }
  }
  resolved = true;
}

set<Variable*> StructMem::getReads()
{
  return base->getReads();
}

set<Variable*> StructMem::getWrites()
{
  return base->getWrites();
}

bool operator==(const StructMem& lhs, const StructMem& rhs)
{
  if(lhs.base != rhs.base)
    return false;
  if(lhs.member.is<Variable*>() != rhs.member.is<Variable*>())
    return false;
  if(lhs.member.is<Variable*>())
    return lhs.member.get<Variable*>()->id == rhs.member.get<Variable*>()->id;
  else
    return lhs.member.get<Subroutine*>()->id == rhs.member.get<Subroutine*>()->id;
}

bool operator<(const StructMem& lhs, const StructMem& rhs)
{
  if(lhs.base < rhs.base)
    return true;
  else if(lhs.base > rhs.base)
    return false;
  else if(lhs.member.is<Variable*>() && rhs.member.is<Subroutine*>())
    return true;
  else if(lhs.member.is<Subroutine*>() && rhs.member.is<Variable*>())
    return false;
  else if(lhs.member.is<Variable*>())
    return lhs.member.get<Variable*>()->id < rhs.member.get<Variable*>()->id;
  else
    return lhs.member.get<Subroutine*>()->id < rhs.member.get<Subroutine*>()->id;
}

/************
 * NewArray *
 ************/

NewArray::NewArray(Type* elemType, vector<Expression*> dimensions)
{
  elem = elemType;
  dims = dimensions;
}

void NewArray::resolveImpl()
{
  resolveType(elem);
  for(size_t i = 0; i < dims.size(); i++)
  {
    resolveExpr(dims[i]);
    if(!dims[i]->type->isInteger())
    {
      errMsgLoc(dims[i], "array dimensions must be integers");
    }
  }
  type = getArrayType(elem, dims.size());
  resolved = true;
}

bool operator==(const NewArray& lhs, const NewArray& rhs)
{
  if(lhs.type != rhs.type)
    return false;
  if(lhs.dims.size() != rhs.dims.size())
    return false;
  for(size_t i = 0; i < lhs.dims.size(); i++)
  {
    if(lhs.dims[i] != rhs.dims[i])
      return false;
  }
  return true;
}

bool operator<(const NewArray& lhs, const NewArray& rhs)
{
  //TODO: implement comparison of types
  if(lhs.type < rhs.type)
    return true;
  else if(lhs.type > rhs.type)
    return false;
  //same types, so just compare the dimensions lexicographically
  for(size_t i = 0; i < lhs.dims.size(); i++)
  {
    if(lhs.dims[i] < rhs.dims[i])
      return true;
    else if(lhs.dims[i] > rhs.dims[i])
      return false;
  }
  return false;
}

/***************
 * ArrayLength *
 ***************/

ArrayLength::ArrayLength(Expression* arr)
{
  array = arr;
}

void ArrayLength::resolveImpl()
{
  resolveExpr(array);
  if(!array->type->isArray())
  {
    //len is not a keyword: <expr>.len is a special case
    //that should be handled in resolveExpr
    INTERNAL_ERROR;
  }
  type = primitives[Prim::LONG];
  resolved = true;
}

set<Variable*> ArrayLength::getReads()
{
  return array->getReads();
}

bool operator==(const ArrayLength& lhs, const ArrayLength& rhs)
{
  return lhs.array == rhs.array;
}

bool operator<(const ArrayLength& lhs, const ArrayLength& rhs)
{
  return lhs.array < rhs.array;
}

void IsExpr::resolveImpl()
{
  resolveExpr(base);
  resolveType(option);
  ut = dynamic_cast<UnionType*>(base->type);
  if(!ut)
  {
    errMsgLoc(this, "is can only be used with a union type");
  }
  //make sure option is actually one of the types in the union
  for(size_t i = 0; i < ut->options.size(); i++)
  {
    if(ut->options[i] == option)
    {
      optionIndex = i;
      resolved = true;
      return;
    }
  }
}

bool operator==(const IsExpr& lhs, const IsExpr& rhs)
{
  return lhs.base == rhs.base && lhs.optionIndex == rhs.optionIndex;
}

bool operator<(const IsExpr& lhs, const IsExpr& rhs)
{
  if(lhs.base < rhs.base)
    return true;
  else if(lhs.base > rhs.base)
    return false;
  else if(lhs.optionIndex < rhs.optionIndex)
    return true;
  return false;
}

void AsExpr::resolveImpl()
{
  resolveExpr(base);
  resolveType(option);
  ut = dynamic_cast<UnionType*>(base->type);
  if(!ut)
  {
    errMsgLoc(this, "as can only be used with a union type");
  }
  //make sure option is actually one of the types in the union
  for(size_t i = 0; i < ut->options.size(); i++)
  {
    if(ut->options[i] == option)
    {
      optionIndex = i;
      resolved = true;
      return;
    }
  }
}

bool operator==(const AsExpr& lhs, const AsExpr& rhs)
{
  return lhs.base == rhs.base && lhs.optionIndex == rhs.optionIndex;
}

bool operator<(const AsExpr& lhs, const AsExpr& rhs)
{
  if(lhs.base < rhs.base)
    return true;
  else if(lhs.base > rhs.base)
    return false;
  else if(lhs.optionIndex < rhs.optionIndex)
    return true;
  return false;
}

/************
 * ThisExpr *
 ************/

ThisExpr::ThisExpr(Scope* where)
{
  //figure out which struct "this" refers to,
  //or show error if there is none
  structType = where->getStructContext();
  if(!structType)
  {
    errMsgLoc(this, "can't use 'this' in static context");
  }
  type = structType;
  resolved = true;
}

UnionConstant::UnionConstant(Expression* expr, Type* t, UnionType* ut)
{
  INTERNAL_ASSERT(expr->constant());
  value = expr;
  unionType = ut;
  type = unionType;
  option = -1;
  for(size_t i = 0; i < ut->options.size(); i++)
  {
    if(t == ut->options[i])
    {
      option = i;
      break;
    }
  }
  resolved = true;
}

/*************
 * Converted *
 *************/

Converted::Converted(Expression* val, Type* dst)
{
  if(!val->resolved)
  {
    INTERNAL_ERROR;
  }
  value = val;
  type = dst;
  resolved = true;
  //converted has same location as original value
  setLocation(val);
  if(!type->canConvert(value->type))
  {
    errMsgLoc(this, "can't implicitly convert from " << \
        val->type->getName() << " to " << type->getName());
  }
}

set<Variable*> Converted::getReads()
{
  return value->getReads();
}

bool operator==(const Converted& lhs, const Converted& rhs)
{
  return lhs.type == rhs.type && lhs.value == rhs.value;
}

bool operator<(const Converted& lhs, const Converted& rhs)
{
  if(lhs.type < rhs.type)
    return true;
  else if(lhs.type > rhs.type)
    return false;
  else if(lhs.value < rhs.value)
    return true;
  return false;
}

/************
 * EnumExpr *
 ************/

EnumExpr::EnumExpr(EnumConstant* ec)
{
  type = ec->et;
  value = ec;
  resolved = true;
}

bool operator==(const EnumExpr& lhs, const EnumExpr& rhs)
{
  return lhs.value == rhs.value;
}

bool operator<(const EnumExpr& lhs, const EnumExpr& rhs)
{
  if(lhs.type < rhs.type)
    return true;
  else if(lhs.type > rhs.type)
    return false;
  EnumType* en = (EnumType*) lhs.type;
  if(en->underlying->isSigned)
    return lhs.value->sval < rhs.value->sval;
  else
    return lhs.value->uval < rhs.value->uval;
}

/*********
 * Error *
 *********/

ErrorVal::ErrorVal()
{
  type = primitives[Prim::ERROR];
  resolved = true;
}

/*************************/
/* Expression resolution */
/*************************/

UnresolvedExpr::UnresolvedExpr(string n, Scope* s)
{
  base = nullptr;
  name = new Member;
  name->names.push_back(n);
  usage = s;
}

UnresolvedExpr::UnresolvedExpr(Member* n, Scope* s)
{
  base = nullptr;
  name = n;
  usage = s;
}

UnresolvedExpr::UnresolvedExpr(Expression* b, Member* n, Scope* s)
{
  base = b;
  name = n;
  usage = s;
}

void resolveExpr(Expression*& expr)
{
  auto unres = dynamic_cast<UnresolvedExpr*>(expr);
  if(!unres)
  {
    expr->resolve();
    return;
  }
  Expression* base = unres->base; //might be null
  //set initial searchScope:
  //the struct scope if base is a struct, otherwise just usage
  size_t nameIter = 0;
  vector<string>& names = unres->name->names;
  //first, get a base expression
  if(!base)
  {
    Scope* baseSearch = unres->usage;
    while(!base)
    {
      Name found = baseSearch->findName(names[nameIter]);
      if(!found.item)
      {
        string fullPath = names[0];
        for(int i = 0; i < nameIter; i++)
        {
          fullPath = fullPath + '.' + names[i];
        }
        errMsgLoc(unres, "unknown identifier " << fullPath);
      }
      //based on type of name, either set base or update search scope
      switch(found.kind)
      {
        case Name::MODULE:
          baseSearch = ((Module*) found.item)->scope;
          break;
        case Name::STRUCT:
          baseSearch = ((StructType*) found.item)->scope;
          break;
        case Name::SUBROUTINE:
          {
            auto subr = (Subroutine*) found.item;
            if(subr->type->ownerStruct)
            {
              //is a member subroutine, so create implicit "this"
              ThisExpr* subrThis = new ThisExpr(unres->usage);
              subrThis->resolve();
              //this must match owner type of subr
              if(subr->type->ownerStruct != subrThis->structType)
              {
                errMsgLoc(unres,
                    "implicit 'this' here can't be used to call " <<
                    subr->type->ownerStruct->name << '.' << subr->name);
              }
              base = new SubroutineExpr(subrThis, (Subroutine*) found.item);
            }
            else
            {
              //nonmember subroutine can be called from anywhere,
              //so no context checking here
              base = new SubroutineExpr(subr, unres->usage);
            }
            break;
          }
        case Name::EXTERN_SUBR:
          base = new SubroutineExpr((ExternalSubroutine*) found.item);
          break;
        case Name::VARIABLE:
          {
            auto var = (Variable*) found.item;
            if(var->owner)
            {
              ThisExpr* varThis = new ThisExpr(unres->usage);
              if(varThis->structType != var->owner)
              {
                errMsgLoc(unres,
                    "implicit 'this' here can't be used to access " <<
                    var->owner->name << '.' << var->name);
              }
              base = new StructMem(varThis, var);
            }
            else
            {
              //static variable can be accessed anywhere
              base = new VarExpr(var);
            }
            break;
          }
        case Name::ENUM_CONSTANT:
          base = new EnumExpr((EnumConstant*) found.item);
          break;
        default:
          errMsgLoc(unres, "identifier is not a valid expression");
      }
      nameIter++;
    }
  }
  base->resolve();
  //base must be resolved (need its type) to continue
  //look up members in searchScope until a new expr can be formed
  while(nameIter < names.size())
  {
    if(base->type->isArray() && names[nameIter] == "len")
    {
      base = new ArrayLength(base);
      //this resolution can't fail
      base->resolve();
      nameIter++;
      continue;
    }
    auto baseStruct = dynamic_cast<StructType*>(base->type);
    if(!baseStruct)
    {
      errMsgLoc(unres, "cant access member of non-struct type " << base->type->getName());
    }
    bool validBase = false;
    Scope* baseSearch = baseStruct->scope;
    while(!validBase && nameIter < names.size())
    {
      Name found = baseSearch->findName(names[nameIter]);
      if(!found.item)
      {
        string fullPath = names[0];
        for(int i = 0; i < nameIter; i++)
        {
          fullPath = fullPath + '.' + names[i];
        }
        errMsgLoc(unres, "unknown identifier " << fullPath);
      }
      //based on type of name, either set base or update search scope
      switch(found.kind)
      {
        case Name::MODULE:
          baseSearch = ((Module*) found.item)->scope;
          break;
        case Name::SUBROUTINE:
          base = new SubroutineExpr((Subroutine*) found.item, unres->usage);
          validBase = true;
          break;
        case Name::VARIABLE:
          base = new StructMem(base, (Variable*) found.item);
          validBase = true;
          break;
        default:
          errMsgLoc(unres, "identifier " << names[nameIter] <<
              " is not a valid member of struct " << base->type->getName());
      }
    }
    if(!validBase)
    {
      //used up all the names but ended up with a module, not an expr
      string fullPath = names[0];
      for(int i = 0; i < nameIter; i++)
      {
        fullPath = fullPath + '.' + names[i];
      }
      errMsgLoc(unres, fullPath << " is not an expression");
    }
    base->resolve();
  }
  //save lexical location of original parsed expression
  base->setLocation(expr);
  expr = base;
}

bool operator==(const Expression& l, const Expression& r)
{
  const Expression* lhs = &l;
  const Expression* rhs = &r;
  if(lhs->getTypeTag() != rhs->getTypeTag())
    return false;
  //now have to compare the individual types of expressions
  //(know that they are the same type)
  if(auto icLHS = dynamic_cast<const IntConstant*>(lhs))
  {
    auto icRHS = dynamic_cast<const IntConstant*>(rhs);
    return *icLHS == *icRHS;
  }
  else if(auto fcLHS = dynamic_cast<const FloatConstant*>(lhs))
  {
    auto fcRHS = dynamic_cast<const FloatConstant*>(rhs);
    return *fcLHS == *fcRHS;
  }
  else if(auto scLHS = dynamic_cast<const StringConstant*>(lhs))
  {
    auto scRHS = dynamic_cast<const StringConstant*>(rhs);
    return *scLHS == *scRHS;
  }
  else if(auto ccLHS = dynamic_cast<const CharConstant*>(lhs))
  {
    auto ccRHS = dynamic_cast<const CharConstant*>(rhs);
    return *ccLHS == *ccRHS;
  }
  else if(auto bcLHS = dynamic_cast<const BoolConstant*>(lhs))
  {
    auto bcRHS = dynamic_cast<const BoolConstant*>(rhs);
    return *bcLHS == *bcRHS;
  }
  else if(auto mcLHS = dynamic_cast<const MapConstant*>(lhs))
  {
    auto mcRHS = dynamic_cast<const MapConstant*>(rhs);
    return *mcLHS == *mcRHS;
  }
  else if(auto clLHS = dynamic_cast<const CompoundLiteral*>(lhs))
  {
    auto clRHS = dynamic_cast<const CompoundLiteral*>(rhs);
    return *clLHS == *clRHS;
  }
  else if(auto ucLHS = dynamic_cast<const UnionConstant*>(lhs))
  {
    auto ucRHS = dynamic_cast<const UnionConstant*>(rhs);
    return *ucLHS == *ucRHS;
  }
  else if(auto uaLHS = dynamic_cast<const UnaryArith*>(lhs))
  {
    auto uaRHS = dynamic_cast<const UnaryArith*>(rhs);
    return *uaLHS == *uaRHS;
  }
  else if(auto baLHS = dynamic_cast<const BinaryArith*>(lhs))
  {
    auto baRHS = dynamic_cast<const BinaryArith*>(rhs);
    return *baLHS == *baRHS;
  }
  else if(auto indLHS = dynamic_cast<const Indexed*>(lhs))
  {
    auto indRHS = dynamic_cast<const Indexed*>(rhs);
    return *indLHS == *indRHS;
  }
  else if(auto naLHS = dynamic_cast<const NewArray*>(lhs))
  {
    auto naRHS = dynamic_cast<const NewArray*>(rhs);
    return *naLHS == *naRHS;
  }
  else if(auto alLHS = dynamic_cast<const ArrayLength*>(lhs))
  {
    auto alRHS = dynamic_cast<const ArrayLength*>(rhs);
    return *alLHS == *alRHS;
  }
  else if(auto asLHS = dynamic_cast<const AsExpr*>(lhs))
  {
    auto asRHS = dynamic_cast<const AsExpr*>(rhs);
    return *asLHS == *asRHS;
  }
  else if(auto isLHS = dynamic_cast<const IsExpr*>(lhs))
  {
    auto isRHS = dynamic_cast<const IsExpr*>(rhs);
    return *isLHS == *isRHS;
  }
  else if(auto callLHS = dynamic_cast<const CallExpr*>(lhs))
  {
    auto callRHS = dynamic_cast<const CallExpr*>(rhs);
    return *callLHS == *callRHS;
  }
  else if(auto varLHS = dynamic_cast<const VarExpr*>(lhs))
  {
    auto varRHS = dynamic_cast<const VarExpr*>(rhs);
    return *varLHS == *varRHS;
  }
  else if(auto convLHS = dynamic_cast<const Converted*>(lhs))
  {
    auto convRHS = dynamic_cast<const Converted*>(rhs);
    return *convLHS == *convRHS;
  }
  else if(dynamic_cast<const ThisExpr*>(lhs) || dynamic_cast<const ErrorVal*>(lhs))
  {
    return true;
  }
  else
  {
    INTERNAL_ERROR;
  }
  return false;
}

bool operator<(const Expression& l, const Expression& r)
{
  const Expression* lhs = &l;
  const Expression* rhs = &r;
  if(lhs->getTypeTag() < rhs->getTypeTag())
    return true;
  else if(lhs->getTypeTag() > rhs->getTypeTag())
    return false;
  //now have to compare the individual types of expressions
  //(know that they are the same type)
  if(auto icLHS = dynamic_cast<const IntConstant*>(lhs))
  {
    auto icRHS = dynamic_cast<const IntConstant*>(rhs);
    return *icLHS < *icRHS;
  }
  else if(auto fcLHS = dynamic_cast<const FloatConstant*>(lhs))
  {
    auto fcRHS = dynamic_cast<const FloatConstant*>(rhs);
    return *fcLHS < *fcRHS;
  }
  else if(auto scLHS = dynamic_cast<const StringConstant*>(lhs))
  {
    auto scRHS = dynamic_cast<const StringConstant*>(rhs);
    return *scLHS < *scRHS;
  }
  else if(auto ccLHS = dynamic_cast<const CharConstant*>(lhs))
  {
    auto ccRHS = dynamic_cast<const CharConstant*>(rhs);
    return *ccLHS < *ccRHS;
  }
  else if(auto bcLHS = dynamic_cast<const BoolConstant*>(lhs))
  {
    auto bcRHS = dynamic_cast<const BoolConstant*>(rhs);
    return *bcLHS < *bcRHS;
  }
  else if(auto mcLHS = dynamic_cast<const MapConstant*>(lhs))
  {
    auto mcRHS = dynamic_cast<const MapConstant*>(rhs);
    return *mcLHS < *mcRHS;
  }
  else if(auto clLHS = dynamic_cast<const CompoundLiteral*>(lhs))
  {
    auto clRHS = dynamic_cast<const CompoundLiteral*>(rhs);
    return *clLHS < *clRHS;
  }
  else if(auto ucLHS = dynamic_cast<const UnionConstant*>(lhs))
  {
    auto ucRHS = dynamic_cast<const UnionConstant*>(rhs);
    return *ucLHS < *ucRHS;
  }
  else if(auto uaLHS = dynamic_cast<const UnaryArith*>(lhs))
  {
    auto uaRHS = dynamic_cast<const UnaryArith*>(rhs);
    return *uaLHS < *uaRHS;
  }
  else if(auto baLHS = dynamic_cast<const BinaryArith*>(lhs))
  {
    auto baRHS = dynamic_cast<const BinaryArith*>(rhs);
    return *baLHS < *baRHS;
  }
  else if(auto indLHS = dynamic_cast<const Indexed*>(lhs))
  {
    auto indRHS = dynamic_cast<const Indexed*>(rhs);
    return *indLHS < *indRHS;
  }
  else if(auto naLHS = dynamic_cast<const NewArray*>(lhs))
  {
    auto naRHS = dynamic_cast<const NewArray*>(rhs);
    return *naLHS < *naRHS;
  }
  else if(auto alLHS = dynamic_cast<const ArrayLength*>(lhs))
  {
    auto alRHS = dynamic_cast<const ArrayLength*>(rhs);
    return *alLHS < *alRHS;
  }
  else if(auto asLHS = dynamic_cast<const AsExpr*>(lhs))
  {
    auto asRHS = dynamic_cast<const AsExpr*>(rhs);
    return *asLHS < *asRHS;
  }
  else if(auto isLHS = dynamic_cast<const IsExpr*>(lhs))
  {
    auto isRHS = dynamic_cast<const IsExpr*>(rhs);
    return *isLHS < *isRHS;
  }
  else if(auto callLHS = dynamic_cast<const CallExpr*>(lhs))
  {
    auto callRHS = dynamic_cast<const CallExpr*>(rhs);
    return *callLHS < *callRHS;
  }
  else if(auto varLHS = dynamic_cast<const VarExpr*>(lhs))
  {
    auto varRHS = dynamic_cast<const VarExpr*>(rhs);
    return *varLHS < *varRHS;
  }
  else if(auto convLHS = dynamic_cast<const Converted*>(lhs))
  {
    auto convRHS = dynamic_cast<const Converted*>(rhs);
    return *convLHS < *convRHS;
  }
  else if(dynamic_cast<const ThisExpr*>(lhs) || dynamic_cast<const ErrorVal*>(lhs))
  {
    //always the same, so a < b always false
    return false;
  }
  else
  {
    INTERNAL_ERROR;
  }
  return false;
}

ostream& operator<<(ostream& os, Expression* e)
{
  INTERNAL_ASSERT(e->resolved);
  if(UnaryArith* ua = dynamic_cast<UnaryArith*>(e))
  {
    os << operatorTable[ua->op] << ua->expr;
  }
  else if(BinaryArith* ba = dynamic_cast<BinaryArith*>(e))
  {
    os << '(' << ba->lhs << ' ' << operatorTable[ba->op] << ' ' << ba->rhs << ')';
  }
  else if(IntConstant* ic = dynamic_cast<IntConstant*>(e))
  {
    if(ic->isSigned())
      os << ic->sval;
    else
      os << ic->uval;
  }
  else if(FloatConstant* fc = dynamic_cast<FloatConstant*>(e))
  {
    if(fc->isDoublePrec())
      os << fc->dp;
    else
      os << fc->fp;
  }
  else if(StringConstant* sc = dynamic_cast<StringConstant*>(e))
  {
    os << generateCharDotfile('"');
    for(size_t i = 0; i < sc->value.size(); i++)
    {
      os << generateCharDotfile(sc->value[i]);
    }
    os << generateCharDotfile('"');
  }
  else if(CharConstant* cc = dynamic_cast<CharConstant*>(e))
  {
    os << generateCharDotfile('\'') << generateCharDotfile(cc->value) << generateCharDotfile('\'');
  }
  else if(BoolConstant* bc = dynamic_cast<BoolConstant*>(e))
  {
    os << (bc->value ? "true" : "false");
  }
  else if(CompoundLiteral* compLit = dynamic_cast<CompoundLiteral*>(e))
  {
    os << '[';
    for(size_t i = 0; i < compLit->members.size(); i++)
    {
      os << compLit->members[i];
      if(i != compLit->members.size() - 1)
        os << ", ";
    }
    os << ']';
  }
  else if(MapConstant* mc = dynamic_cast<MapConstant*>(e))
  {
    os << '[';
    for(auto it = mc->values.begin(); it != mc->values.end(); it++)
    {
      if(it != mc->values.begin())
      {
        os << ", ";
      }
      os << '{' << it->first << ", " << it->second << '}';
    }
    os << ']';
  }
  else if(UnionConstant* uc = dynamic_cast<UnionConstant*>(e))
  {
    os << uc->value->type->getName() << ": " << uc->value;
  }
  else if(Indexed* in = dynamic_cast<Indexed*>(e))
  {
    os << in->group << '[' << in->index << ']';
  }
  else if(CallExpr* call = dynamic_cast<CallExpr*>(e))
  {
    os << call->callable << '(';
    for(size_t i = 0; i < call->args.size(); i++)
    {
      os << call->args[i];
      if(i != call->args.size() - 1)
        os << ", ";
    }
    os << ')';
  }
  else if(VarExpr* ve = dynamic_cast<VarExpr*>(e))
  {
    os << ve->var->name;
  }
  else if(NewArray* na = dynamic_cast<NewArray*>(e))
  {
    os << "array " << na->elem->getName();
    for(auto dim : na->dims)
    {
      os << '[' << dim << ']';
    }
  }
  else if(Converted* c = dynamic_cast<Converted*>(e))
  {
    os << '(' << c->type->getName();
    os << ") (" << c->value << ')';
  }
  else if(IsExpr* ie = dynamic_cast<IsExpr*>(e))
  {
    os << '(' << ie->base << " is " << ie->option->getName() << ')';
  }
  else if(AsExpr* ae = dynamic_cast<AsExpr*>(e))
  {
    os << '(' << ae->base << " as " << ae->option->getName() << ')';
  }
  else if(ArrayLength* al = dynamic_cast<ArrayLength*>(e))
  {
    os << '(' << al->array << ").len";
  }
  else if(dynamic_cast<ThisExpr*>(e))
  {
    os << "this";
  }
  else if(dynamic_cast<ErrorVal*>(e))
  {
    os << "error";
  }
  return os;
}

