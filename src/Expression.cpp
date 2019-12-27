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
  if(op == LNOT && !typesSame(expr->type, primitives[Prim::BOOL]))
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

Expression* UnaryArith::copy()
{
  auto c = new UnaryArith(op, expr->copy());
  c->resolve();
  return c;
}

bool UnaryArith::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const UnaryArith*>(&erhs);
  if(!rhs)
    return false;
  return op == rhs->op && *expr == *rhs->expr;
}

ostream& UnaryArith::print(ostream& os)
{
  os << operatorTable[op] << expr;
  return os;
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
      if(!typesSame(ltype, primitives[Prim::BOOL]) ||
         !typesSame(rtype, primitives[Prim::BOOL]))
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
      if(!typesSame(ltype, type))
      {
        lhs = new Converted(lhs, type);
      }
      if(!typesSame(rtype, type))
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
        if(!typesSame(ltype, type))
        {
          lhs = new Converted(lhs, type);
        }
        if(!typesSame(rtype, type))
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
        if(!typesSame(subtype, rtype))
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
        if(!typesSame(subtype, ltype))
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
      if(!(ltype->isNumber()) || !(rtype->isNumber()))
      {
        errMsgLoc(this, "operands to arithmetic operators must be numbers.");
      }
      type = promote(ltype, rtype);
      if(!typesSame(ltype, type))
      {
        lhs = new Converted(lhs, type);
      }
      if(!typesSame(rtype, type))
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
      if(!typesSame(ltype, rtype))
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
      //now, both types are identical
      //check for map relational comparison (the only kind not supported)
      bool relational = !(op == CMPEQ || op == CMPNEQ);
      if(relational && lhs->type->isMap())
      {
        errMsgLoc(this, "maps can't be compared with the relational operators");
      }
      break;
    }
    default: INTERNAL_ERROR;
  }
  resolved = true;
}

Expression* BinaryArith::copy()
{
  auto c = new BinaryArith(lhs->copy(), op, rhs->copy());
  c->resolve();
  return c;
}

bool BinaryArith::operator==(const Expression& eother) const
{
  auto other = dynamic_cast<const BinaryArith*>(&eother);
  if(!other)
    return false;
  if(op != other->op)
    return false;
  if(*lhs == *other->lhs && *rhs == *other->rhs)
    return true;
  if(operCommutativeTable[op])
  {
    if(*lhs == *other->rhs && *rhs == *other->lhs)
      return true;
  }
  return false;
}

ostream& BinaryArith::print(ostream& os)
{
  os << '(' << lhs << ' ' << operatorTable[op];
  os << ' ' << rhs << ')';
  return os;
}

/***************
 * IntConstant *
 ***************/

Expression* IntConstant::convert(Type* t)
{
  if(auto dstType = dynamic_cast<IntegerType*>(t))
  {
    //just give this constant the same value,
    //then make sure the value fits
    IntConstant* intConstant = new IntConstant;
    intConstant->setLocation(this);
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
    //make sure value is actually in the enum
    if(isSigned())
    {
      for(auto ec : enumType->values)
      {
        if(sval == (int64_t) ec->value)
          return new EnumExpr(ec);
      }
    }
    else
    {
      for(auto ec : enumType->values)
      {
        if(uval == ec->value)
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
    FloatConstant* fc = nullptr;
    if(floatType->size == 4)
    {
      if(isSigned())
        fc = new FloatConstant((float) sval);
      else
        fc = new FloatConstant((float) uval);
    }
    else
    {
      if(isSigned())
        fc = new FloatConstant((double) sval);
      else
        fc = new FloatConstant((double) uval);
    }
    fc->setLocation(this);
    return fc;
  }
  else if(dynamic_cast<CharType*>(t))
  {
    auto charInt = (IntegerType*) primitives[Prim::UBYTE];
    CharConstant* cc = nullptr;
    if(isSigned())
    {
      if(sval >= 0 && sval <= (int64_t) charInt->maxUnsignedVal())
        cc = new CharConstant((char) sval);
    }
    else
    {
      if(uval <= charInt->maxUnsignedVal())
        cc = new CharConstant((char) uval);
    }
    if(!cc)
      errMsgLoc(this, "integer value doesn't fit in char");
    cc->setLocation(this);
    return cc;
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
      result->uval = uval op rhs->uval; \
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
          result->sval = sval / rhs->sval;
        else
          result->uval = uval / rhs->uval;
      }
      else
      {
        if(isSigned())
          result->sval = sval % rhs->sval;
        else
          result->uval = uval % rhs->uval;
      }
      break;
    }
    DO_OP(BOR, |)
    DO_OP(BXOR, ^)
    DO_OP(BAND, &)
    case SHL:
    case SHR:
    {
      int64_t shiftBits = rhs->uval;
      if(rhs->isSigned())
      {
        if(rhs->sval < 0)
        {
          errMsg("Shifting by negative number of bits is illegal.");
        }
        shiftBits = rhs->sval;
      }
      if(isSigned())
      {
        if(op == SHL)
          result->sval = sval << shiftBits;
        else
          result->sval = sval >> shiftBits;
      }
      else
      {
        if(op == SHL)
          result->uval = uval << shiftBits;
        else
          result->uval = uval >> shiftBits;
      }
      break;
    }
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

Expression* IntConstant::copy()
{
  auto c = new IntConstant;
  c->type = type;
  c->sval = sval;
  c->uval = uval;
  c->resolve();
  return c;
}

bool IntConstant::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const IntConstant*>(&erhs);
  if(!rhs)
    return false;
  if(!typesSame(type, rhs->type))
    return false;
  if(isSigned())
    return sval == rhs->sval;
  return uval == rhs->uval;
}

ostream& IntConstant::print(ostream& os)
{
  if(isSigned())
    os << sval;
  else
    os << uval;
  return os;
}

/*****************
 * FloatConstant *
 *****************/

Expression* FloatConstant::convert(Type* t)
{
  //first, just promote to double
  double val = typesSame(type, primitives[Prim::FLOAT]) ? fp : dp;
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
      asLong.setLocation(this);
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
      asULong.setLocation(this);
      return asULong.convert(t);
    }
  }
  else if(auto floatType = dynamic_cast<FloatType*>(t))
  {
    FloatConstant* fc = nullptr;
    if(floatType->size == 4)
      fc = new FloatConstant((float) val);
    else
      fc = new FloatConstant(val);
    fc->setLocation(this);
    return fc;
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
  result->setLocation(this);
  return result;
}

Expression* FloatConstant::copy()
{
  auto c = new FloatConstant;
  c->type = type;
  c->fp = fp;
  c->dp = dp;
  c->setLocation(this);
  return c;
}

bool FloatConstant::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const FloatConstant*>(&erhs);
  if(!rhs)
    return false;
  if(!typesSame(type, rhs->type))
    return false;
  if(isDoublePrec())
    return dp == rhs->dp;
  return fp == rhs->fp;
}

ostream& FloatConstant::print(ostream& os)
{
  if(isDoublePrec())
    os << dp;
  else
    os << fp;
  return os;
}

/******************
 * StringConstant *
 ******************/

Expression* StringConstant::copy()
{
  auto sc = new StringConstant(value);
  sc->setLocation(this);
  return sc;
}

bool StringConstant::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const StringConstant*>(&erhs);
  if(!rhs)
    return false;
  return value == rhs->value;
}

Expression* CharConstant::copy()
{
  auto cc = new CharConstant(value);
  cc->setLocation(this);
  return cc;
}

ostream& StringConstant::print(ostream& os)
{
  os << generateCharDotfile('"');
  for(size_t i = 0; i < value.size(); i++)
  {
    os << generateCharDotfile(value[i]);
  }
  os << generateCharDotfile('"');
  return os;
}

/****************
 * CharConstant *
 ****************/

bool CharConstant::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const CharConstant*>(&erhs);
  if(!rhs)
    return false;
  return value == rhs->value;
}

ostream& CharConstant::print(ostream& os)
{
  os << generateCharDotfile('\'') << generateCharDotfile(value) << generateCharDotfile('\'');
  return os;
}

/****************
 * BoolConstant *
 ****************/

Expression* BoolConstant::copy()
{
  auto bc = new BoolConstant(value);
  bc->setLocation(this);
  return bc;
}

bool BoolConstant::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const BoolConstant*>(&erhs);
  if(!rhs)
    return false;
  return value == rhs->value;
}

ostream& BoolConstant::print(ostream& os)
{
  os << (value ? "true" : "false");
  return os;
}

/***************
 * MapConstant *
 ***************/

MapConstant::MapConstant(MapType* mt)
{
  type = mt;
  resolved = true;
}

Expression* MapConstant::copy()
{
  MapConstant* c = new MapConstant((MapType*) type);
  //have to deep copy all the keys/values
  for(auto& kv : values)
  {
    c->values[kv.first->copy()] = kv.second->copy();
  }
  c->setLocation(this);
  return c;
}

bool MapConstant::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const MapConstant*>(&erhs);
  if(!rhs)
    return false;
  auto& l = values;
  auto& r = rhs->values;
  if(l.size() != r.size())
    return false;
  //iterate through lhs elements, look up in rhs
  for(auto lkv : l)
  {
    auto it = r.find(lkv.first);
    if(it == r.end() || *lkv.second != *it->second)
      return false;
  }
  return true;
}

ostream& MapConstant::print(ostream& os)
{
  os << '[';
  for(auto it = values.begin(); it != values.end(); it++)
  {
    if(it != values.begin())
    {
      os << ", ";
    }
    os << '{' << it->first << ", " << it->second << '}';
  }
  os << ']';
  return os;
}

/*****************
 * UnionConstant *
 *****************/

UnionConstant::UnionConstant(Expression* expr, UnionType* ut)
{
  INTERNAL_ASSERT(expr->resolved && expr->constant());
  setLocation(expr);
  value = expr;
  unionType = ut;
  type = unionType;
  option = -1;
  for(size_t i = 0; i < ut->options.size(); i++)
  {
    if(typesSame(expr->type, ut->options[i]))
    {
      option = i;
      break;
    }
  }
  resolved = true;
}

Expression* UnionConstant::copy()
{
  auto uc = new UnionConstant(value->copy(), unionType);
  uc->setLocation(this);
  return uc;
}

bool UnionConstant::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const UnionConstant*>(&erhs);
  if(!rhs)
    return false;
  return option == rhs->option && *value == *rhs->value;
}

ostream& UnionConstant::print(ostream& os)
{
  if(value->type->isSimple())
    os << value;
  else
    os << value->type->getName() << ": " << value;
  return os;
}

/*******************
 * CompoundLiteral *
 *******************/

CompoundLiteral::CompoundLiteral(vector<Expression*>& mems)
  : members(mems) {}

CompoundLiteral::CompoundLiteral(vector<Expression*>& mems, Type* t)
  : members(mems)
{
  type = t;
  resolved = true;
}

void CompoundLiteral::resolveImpl()
{
  if(resolved)
    return;
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

Expression* CompoundLiteral::copy()
{
  vector<Expression*> memsCopy;
  for(auto m : members)
    memsCopy.push_back(m->copy());
  CompoundLiteral* c = new CompoundLiteral(memsCopy, type);
  c->setLocation(this);
  return c;
}

bool CompoundLiteral::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const CompoundLiteral*>(&erhs);
  if(!rhs)
    return false;
  auto& l = members;
  auto& r = rhs->members;
  if(l.size() != r.size())
    return false;
  for(size_t i = 0; i < l.size(); i++)
  {
    if(*l[i] != *r[i])
      return false;
  }
  return true;
}

ostream& CompoundLiteral::print(ostream& os)
{
  if(constant() && type == getArrayType(primitives[Prim::CHAR], 1))
  {
    //it's a string, so just print it as a string literal
    os << generateCharDotfile('"');
    for(size_t i = 0; i < members.size(); i++)
    {
      auto scc = dynamic_cast<CharConstant*>(members[i]);
      INTERNAL_ASSERT(scc);
      os << generateCharDotfile(scc->value);
    }
    os << generateCharDotfile('"');
  }
  else
  {
    os << '[';
    for(size_t i = 0; i < members.size(); i++)
    {
      os << members[i];
      if(i != members.size() - 1)
        os << ", ";
    }
    os << ']';
  }
  return os;
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
    if(!intIndex)
      errMsgLoc(this, "tuple subscript must be an integer constant.");
    uint64_t idx = intIndex->uval;
    bool outOfBounds = false;
    if(intIndex->isSigned())
    {
      if(intIndex->sval < 0)
        outOfBounds = true;
      else
        idx = intIndex->sval;
    }
    if(idx >= tt->members.size())
      outOfBounds = true;
    if(outOfBounds)
      errMsgLoc(this, "tuple subscript out of bounds");
    type = tt->members[idx];
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

Expression* Indexed::copy()
{
  Indexed* c = new Indexed(group->copy(), index->copy());
  c->resolve();
  c->setLocation(this);
  return c;
}

bool Indexed::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const Indexed*>(&erhs);
  if(!rhs)
    return false;
  return *group == *rhs->group && *index == *rhs->index;
}

ostream& Indexed::print(ostream& os)
{
  os << group << '[' << index << ']';
  return os;
}

/************
 * CallExpr *
 ************/

CallExpr::CallExpr(Expression* c, vector<Expression*>& a)
{
  callable = c;
  args = a;
}

void CallExpr::resolveImpl()
{
  resolveExpr(callable);
  //Resolve arguments
  vector<Type*> argTypes(args.size());
  for(size_t i = 0; i < args.size(); i++)
  {
    resolveExpr(args[i]);
    argTypes[i] = args[i]->type;
  }
  auto soe = dynamic_cast<SubrOverloadExpr*>(callable);
  if(soe)
  {
    //need to choose from a set of overloads based on arg types
    SubrBase* sb = soe->decl->match(argTypes);
    if(!sb)
      errMsgLoc(this, "No overloads of " << soe->decl->name << " match arg types");
    //replace callable
    if(soe->thisObject)
    {
      Subroutine* subr = dynamic_cast<Subroutine*>(sb);
      INTERNAL_ASSERT(subr);
      callable = new StructMem(soe->thisObject, subr);
    }
    else
      callable = new SubroutineExpr(sb);
    callable->resolve();
    callable->setLocation(this);
  }
  //have a specific callable
  auto callableType = dynamic_cast<CallableType*>(callable->type);
  if(!callableType)
  {
    errMsgLoc(this, "Expression of type " << callable->type->getName() << " is not callable");
  }
  type = callableType->returnType;
  //make sure number of arguments matches
  if(callableType->paramTypes.size() != args.size())
  {
    errMsgLoc(this, "in call to " <<
        (callableType->ownerStruct ? "" : "static") <<
        (callableType->pure ? "function" : "procedure") <<
        ", expected " <<
        callableType->paramTypes.size() <<
        " arguments but " <<
        args.size() << " were provided");
  }
  for(size_t i = 0; i < args.size(); i++)
  {
    //make sure arg value can be converted to expected type
    if(!callableType->paramTypes[i]->canConvert(args[i]->type))
    {
      errMsgLoc(args[i], "argument " << i + 1 << " to " <<
        (callableType->pure ? "function " : "procedure ") << callable <<
        " has wrong type (expected " << callableType->paramTypes[i]->getName()
        << " but got " << args[i]->type->getName());
    }
    if(!typesSame(callableType->paramTypes[i], args[i]->type))
    {
      Node* loc = args[i];
      args[i] = new Converted(args[i], callableType->paramTypes[i]);
      args[i]->resolve();
      args[i]->setLocation(loc);
    }
  }
  resolved = true;
}

Expression* CallExpr::copy()
{
  vector<Expression*> argsCopy;
  for(auto a : args)
    argsCopy.push_back(a->copy());
  auto c = new CallExpr(callable->copy(), argsCopy);
  c->resolve();
  c->setLocation(this);
  return c;
}

bool CallExpr::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const CallExpr*>(&erhs);
  if(!rhs)
    return false;
  if(*callable != *rhs->callable)
    return false;
  INTERNAL_ASSERT(args.size() == rhs->args.size());
  for(size_t i = 0; i < args.size(); i++)
  {
    if(*args[i] != *rhs->args[i])
      return false;
  }
  return true;
}

ostream& CallExpr::print(ostream& os)
{
  os << callable << '(';
  for(size_t i = 0; i < args.size(); i++)
  {
    os << args[i];
    if(i != args.size() - 1)
      os << ", ";
  }
  os << ')';
  return os;
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
    if(fnScope && !fnScope->contains(varScope))
    {
      errMsgLoc(this, "use of variable " << var->name << " here violates function purity");
    }
  }
  resolved = true;
}

Expression* VarExpr::copy()
{
  auto c = new VarExpr(var);
  c->resolve();
  c->setLocation(this);
  return c;
}

bool VarExpr::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const VarExpr*>(&erhs);
  if(!rhs)
    return false;
  return var == rhs->var;
}

ostream& VarExpr::print(ostream& os)
{
  os << var->name;
  return os;
}

/********************
 * SubrOverloadExpr *
 ********************/

SubrOverloadExpr::SubrOverloadExpr(SubroutineDecl* d)
{
  INTERNAL_ASSERT(d->resolved);
  decl = d;
  thisObject = nullptr;
  //A SubrOverloadExpr has no single type.
  type = nullptr;
  resolved = true;
}

SubrOverloadExpr::SubrOverloadExpr(Expression* t, SubroutineDecl* d)
{
  INTERNAL_ASSERT(d->resolved);
  INTERNAL_ASSERT(t->resolved);
  decl = d;
  thisObject = t;
  //A SubrOverloadExpr never has a single type, even when resovled.
  type = nullptr;
  resolved = true;
}

ostream& SubrOverloadExpr::print(ostream& os)
{
  os << (decl->isPure ? "func " : "proc ") << decl->name;
  return os;
}

/******************
 * SubroutineExpr *
 ******************/

SubroutineExpr::SubroutineExpr(SubrBase* s)
{
  subr = s;
  type = subr->type;
  resolved = true;
}

void SubroutineExpr::resolveImpl()
{
  subr->resolve();
  type = subr->type;
  resolved = true;
}

Expression* SubroutineExpr::copy()
{
  SubroutineExpr* c = new SubroutineExpr(subr);
  c->resolve();
  c->setLocation(this);
  return c;
}

bool SubroutineExpr::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const SubroutineExpr*>(&erhs);
  if(!rhs)
    return false;
  return subr == rhs->subr;
}

ostream& SubroutineExpr::print(ostream& os)
{
  INTERNAL_ASSERT(resolved);
  os << subr->name();
  return os;
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

Expression* StructMem::copy()
{
  StructMem* c = nullptr;
  if(member.is<Variable*>())
    c = new StructMem(base->copy(), member.get<Variable*>());
  else
    c = new StructMem(base->copy(), member.get<Subroutine*>());
  c->resolve();
  c->setLocation(this);
  return c;
}

bool StructMem::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const StructMem*>(&erhs);
  if(!rhs)
    return false;
  if(*base != *rhs->base)
    return false;
  if(member.is<Variable*>() != rhs->member.is<Variable*>())
    return false;
  if(member.is<Variable*>())
    return member.get<Variable*>()->id == rhs->member.get<Variable*>()->id;
  else
    return member.get<Subroutine*>()->id == rhs->member.get<Subroutine*>()->id;
}

ostream& StructMem::print(ostream& os)
{
  os << base << '.';
  if(member.is<Variable*>())
    os << member.get<Variable*>()->name;
  else
    os << member.get<Subroutine*>()->name();
  return os;
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

Expression* NewArray::copy()
{
  vector<Expression*> dimsCopy;
  for(auto d : dims)
    dimsCopy.push_back(d->copy());
  auto c = new NewArray(elem, dimsCopy);
  c->resolve();
  c->setLocation(this);
  return c;
}

bool NewArray::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const NewArray*>(&erhs);
  if(!rhs)
    return false;
  if(!typesSame(type, rhs->type))
    return false;
  if(dims.size() != rhs->dims.size())
    return false;
  for(size_t i = 0; i < dims.size(); i++)
  {
    if(*dims[i] != *rhs->dims[i])
      return false;
  }
  return true;
}

ostream& NewArray::print(ostream& os)
{
  os << "array " << elem->getName();
  for(auto dim : dims)
  {
    os << '[' << dim << ']';
  }
  return os;
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

Expression* ArrayLength::copy()
{
  ArrayLength* c = new ArrayLength(array->copy());
  c->resolve();
  c->setLocation(this);
  return c;
}

bool ArrayLength::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const ArrayLength*>(&erhs);
  if(!rhs)
    return false;
  return *array == *rhs->array;
}

ostream& ArrayLength::print(ostream& os)
{
  os << '(' << array << ").len";
  return os;
}

/**********
 * IsExpr *
 **********/

void IsExpr::resolveImpl()
{
  resolveExpr(base);
  resolveType(option);
  ut = dynamic_cast<UnionType*>(canonicalize(base->type));
  if(!ut)
  {
    errMsgLoc(this, "is can only be used with a union type");
  }
  //make sure option is actually one of the types in the union
  for(size_t i = 0; i < ut->options.size(); i++)
  {
    if(typesSame(ut->options[i], option))
    {
      optionIndex = i;
      resolved = true;
      return;
    }
  }
}

Expression* IsExpr::copy()
{
  auto c = new IsExpr(base->copy(), option);
  c->resolve();
  return c;
}

bool IsExpr::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const IsExpr*>(&erhs);
  if(!rhs)
    return false;
  return *base == *rhs->base && optionIndex == rhs->optionIndex;
}

ostream& IsExpr::print(ostream& os)
{
  os << '(' << base << " is " << option->getName() << ')';
  return os;
}

/**********
 * AsExpr *
 **********/

void AsExpr::resolveImpl()
{
  resolveExpr(base);
  resolveType(type);
  ut = dynamic_cast<UnionType*>(canonicalize(base->type));
  if(!ut)
  {
    errMsgLoc(this, "as can only be used with a union type");
  }
  //make sure option is actually one of the types in the union
  for(size_t i = 0; i < ut->options.size(); i++)
  {
    if(typesSame(ut->options[i], type))
    {
      optionIndex = i;
      resolved = true;
      return;
    }
  }
}

Expression* AsExpr::copy()
{
  auto c = new AsExpr(base->copy(), type);
  c->resolve();
  return c;
}

bool AsExpr::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const AsExpr*>(&erhs);
  if(!rhs)
    return false;
  return *base == *rhs->base && optionIndex == rhs->optionIndex;
}

ostream& AsExpr::print(ostream& os)
{
  os << '(' << base << " as " << type->getName() << ')';
  return os;
}

/************
 * ThisExpr *
 ************/

ThisExpr::ThisExpr(Scope* where)
{
  usage = where;
}

void ThisExpr::resolveImpl()
{
  //figure out which struct "this" refers to,
  //or show error if there is none
  structType = usage->getStructContext();
  if(!structType)
  {
    errMsgLoc(this, "can't use 'this' in static context");
  }
  type = structType;
  resolved = true;
}

Expression* ThisExpr::copy()
{
  return this;
}

ostream& ThisExpr::print(ostream& os)
{
  os << "this";
  return os;
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

Expression* Converted::copy()
{
  auto c = new Converted(value->copy(), type);
  c->resolve();
  return c;
}

bool Converted::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const Converted*>(&erhs);
  if(!rhs)
    return false;
  return typesSame(type, rhs->type) && *value == *rhs->value;
}

ostream& Converted::print(ostream& os)
{
  os << '(' << type->getName();
  os << ") (" << value << ')';
  return os;
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

Expression* EnumExpr::copy()
{
  return this;
}

bool EnumExpr::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const EnumExpr*>(&erhs);
  if(!rhs)
    return false;
  return value == rhs->value;
}

ostream& EnumExpr::print(ostream& os)
{
  os << value->name;
  return os;
}

/******************
 * SimpleConstant *
 ******************/

SimpleConstant::SimpleConstant(SimpleType* s)
{
  st = s;
  type = s;
  resolved = true;
}

Expression* SimpleConstant::copy()
{
  return new SimpleConstant(st);
}

bool SimpleConstant::operator==(const Expression& erhs) const
{
  auto rhs = dynamic_cast<const SimpleConstant*>(&erhs);
  if(!rhs)
    return false;
  return st == rhs->st;
}

ostream& SimpleConstant::print(ostream& os)
{
  os << st->name;
  return os;
}

size_t SimpleConstant::hash() const
{
  return fnv1a(st);
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

void UnresolvedExpr::setShortcutEnum(EnumType* et)
{
  INTERNAL_ASSERT(!shortcutEnum);
  shortcutEnum = et;
}

void UnresolvedExpr::clearShortcutEnum()
{
  shortcutEnum = nullptr;
}

EnumType* UnresolvedExpr::shortcutEnum = nullptr;

/*
 * Expr resolution flow chart:
 * In member context?
 *  Yes:
 *  -Look up name in current scope
 *  -If a scope comes back: continue lookup from that scope
 *  -If something else found, have a base expr
 *    -If a static decl, just handle it as a direct VarExpr or SubrOverloadExpr
 *    -Otherwise, create a StructMem using an implicit "this"
 *  -Otherwise, nothing found: try to find a callable member thru composition
 *    -If something found, continue as above
 *    -If nothing found, try lookup in an enclosing scope (first name only)
 *      -Succeeds and produces a scope, that is the new (only) scope for lookup 
 *      -Succeeds and produces an expr, that's the base
 *      -Fails, last chance is the EnumExpr shortcut
 *        -If THAT fails, error: use of undeclared name
 *  No (static context):
 *  -Look up name in current scope
 *  -If a scope comes back: continue lookup from the found scope (only)
 *  -If something else found, have a base expr
 *    -If a NON-static decl, error since there's no "this" in context
 *    -Otherwise, create a SubrOverloadExpr, VarExpr, EnumExpr, etc.
 *  -If nothing comes back, look up in an enclosing scope (first name only)
 *    -If that produces a scope, search from there only
 *
 *  -Once a base is produced:
 *    -Continue consuming names. Only structs can have named members (except for array ".len")
 *    -So: the search scope for names past base is always the StructType's scope.
 *    -Produce nested StructMems as necessary
 *    -Update search scope as necessary (lookup can only be in current scope + composition now, not enclosing scope)
 */

void resolveExpr(Expression*& expr)
{
  if(auto defaultVal = dynamic_cast<DefaultValueExpr*>(expr))
  {
    resolveType(defaultVal->t);
    expr = defaultVal->t->getDefaultValue();
    INTERNAL_ASSERT(expr->resolved);
    return;
  }
  auto unres = dynamic_cast<UnresolvedExpr*>(expr);
  if(!unres)
  {
    expr->resolve();
    return;
  }
  Expression* base = unres->base; //might be null
  //set initial searchScope:
  //the struct scope if base is a struct, otherwise just usage
  vector<string>& names = unres->name->names;
  Scope* searchScope = unres->usage;
  //need a "base" expression first
  //(could be the whole expr, or could be the root of a StructMem etc.)
  if(names.size() == 1 && UnresolvedExpr::shortcutEnum)
  {
    //only use enum shortcut if no other name matches an expression
    Name n = searchScope->findName(names[0]);
    if(n.item == nullptr ||
        (n.kind != Name::SIMPLE_TYPE &&
         n.kind != Name::SUBROUTINE &&
         n.kind != Name::VARIABLE &&
         n.kind != Name::ENUM_CONSTANT))
    {
      //only name that can appear inside enum is EnumConstant, so no dynamic cast
      EnumExpr* ee = UnresolvedExpr::shortcutEnum->valueFromName(names[0]);
      if(ee)
      {
        Node* loc = unres;
        expr = ee;
        expr->setLocation(loc);
        return;
      }
    }
  }
  //If a base is given, resolve it
  if(base)
  {
    resolveExpr(base);
  }
  //Decide where to start search
  StructType* structContext = unres->usage->getStructContext();
  if(base)
  {
    structContext = dynamic_cast<StructType*>(base->type);
    if(structContext)
      searchScope = structContext->scope;
  }
  //Consume all names, while maintaining base, searchScope and structContext
  for(size_t i = 0; i < names.size(); i++)
  {
    //Handle special case: x.len where x is an array
    if(base && (base->type->isArray() || base->type->isMap()) && names[i] == "len")
    {
      Node* loc = base;
      base = new ArrayLength(base);
      base->resolve();
      base->setLocation(loc);
      continue;
    }
    if(structContext)
    {
      //have explicit base and struct context
      size_t namesUsed = 0;
      //findMember errors out if nothing found
      base = structContext->findMember(base, names.data() + i,
          names.size() - i, namesUsed);
      //compensate for the "++" in the loop
      i += namesUsed - 1;
      structContext = dynamic_cast<StructType*>(base->type);
      if(structContext)
        searchScope = structContext->scope;
      else
        searchScope = nullptr;
    }
    else
    {
      if(!searchScope)
      {
        //There must be a base, otherwise search wouldn't be null
        errMsgLoc(base, "Expression " << base << " can't have named members.");
      }
      //Otherwise, use scope lookup.
      Name found = (i == 0) ? searchScope->findName(names[i]) : searchScope->lookup(names[i]);
      StructType* enclosingStruct = searchScope->getStructContext();
      if(!found.item && i == 0 && enclosingStruct)
      {
        //after normal lookup failed, try to use full struct member lookup
        size_t namesUsed = 0;
        ThisExpr* implicitThis = new ThisExpr(searchScope);
        implicitThis->resolve();
        implicitThis->setLocation(unres);
        base = enclosingStruct->findMember(implicitThis, names.data(), names.size(), namesUsed);
        i += namesUsed - 1;
        //update context for new base
        structContext = dynamic_cast<StructType*>(base->type);
        if(structContext)
          searchScope = structContext->scope;
        else
          searchScope = nullptr;
      }
      else if(!found.item)
      {
        errMsgLoc(unres, "Name " << names[i] << " was not defined in this context.");
      }
      else
      {
        //Found a name.
        switch(found.kind)
        {
          case Name::MODULE:
            structContext = nullptr;
            searchScope = ((Module*) found.item)->scope;
            break;
          case Name::STRUCT:
            //static reference to struct type - just like module
            structContext = nullptr;
            searchScope = ((StructType*) found.item)->scope;
            break;
          case Name::ENUM:
            structContext = nullptr;
            searchScope = ((EnumType*) found.item)->scope;
            break;
          case Name::SIMPLE_TYPE:
            base = ((SimpleType*) found.item)->val;
            base->resolve();
            base->setLocation(unres);
            structContext = nullptr;
            searchScope = nullptr;
            break;
          case Name::SUBROUTINE:
          {
            //Static or implicit this reference to SubroutineDecl
            SubroutineDecl* sd = (SubroutineDecl*) found.item;
            if(sd->owner)
            {
              //Make sure searchScope context matches 
              if(!enclosingStruct)
                errMsgLoc(unres, "Can't call " << sd->name << " (member of " <<
                    sd->owner->name << ") in static context.")
              else if(enclosingStruct != sd->owner)
                errMsgLoc(unres, "Can't call " << sd->name << " (member of " <<
                    sd->owner->name << ") in context where 'this' refers to other struct "
                    << enclosingStruct->name);
              //Can call with implicit this
              ThisExpr* implicitThis = new ThisExpr(searchScope);
              implicitThis->resolve();
              implicitThis->setLocation(unres);
              Subroutine* only = dynamic_cast<Subroutine*>(sd->getOnly());
              if(only)
                base = new StructMem(implicitThis, only);
              else
                base = new SubrOverloadExpr(implicitThis, sd);
            }
            else
            {
              //Calling a static subr is allowed in any context
              SubrBase* only = sd->getOnly();
              if(only)
                base = new SubroutineExpr(only);
              else
                base = new SubrOverloadExpr(sd);
            }
            base->resolve();
            base->setLocation(unres);
            structContext = nullptr;
            searchScope = nullptr;
            break;
          }
          case Name::VARIABLE:
          {
            //Reference to variable or member of implicit this
            Variable* var = (Variable*) found.item;
            if(var->owner)
            {
              if(!enclosingStruct)
              {
                errMsgLoc(unres, "Can't refer to member " << var->name << " of struct " <<
                    var->owner->name << ") in static context.");
              }
              else if(enclosingStruct != var->owner)
              {
                errMsgLoc(unres, "Can't refer to member " << var->name << " of struct " <<
                    var->owner->name << ") where 'this' refers to other struct " << enclosingStruct->name);
              }
              ThisExpr* implicitThis = new ThisExpr(searchScope);
              implicitThis->resolve();
              implicitThis->setLocation(unres);
              base = new StructMem(implicitThis, var);
              base->resolve();
              structContext = dynamic_cast<StructType*>(base->type);
              if(structContext)
                searchScope = structContext->scope;
              else
                searchScope = nullptr;
            }
            else
            {
              base = new VarExpr(var, searchScope);
              base->resolve();
            }
            base->setLocation(unres);
            break;
          }
          case Name::ENUM_CONSTANT:
            base = new EnumExpr((EnumConstant*) found.item);
            base->resolve();
            base->setLocation(unres);
            structContext = nullptr;
            searchScope = nullptr;
            break;
          case Name::TYPEDEF:
            errMsgLoc(unres, "Expected expression but got alias type name " << names[i]);
            break;
          default:
            //Want to catch new Kinds being added
            INTERNAL_ERROR;
        }
      }
    }
  }
  expr = base;
  INTERNAL_ASSERT(expr->resolved);
}

void resolveAndCoerce(Expression*& expr, Type* reqType)
{
  resolveExpr(expr);
  SubrOverloadExpr* soe = dynamic_cast<SubrOverloadExpr*>(expr);
  if(soe)
  {
    auto callable = dynamic_cast<CallableType*>(reqType);
    if(!callable)
      errMsgLoc(expr, "Reference to subroutine " << soe->decl->name <<
          " can't be converted to non-callable type " << reqType->getName());
    Expression* thisObject = soe->thisObject;
    SubrBase* sb = soe->decl->match(callable);
    if(!sb)
    {
      //give error message for why match failed
      if(soe->decl->owner != callable->ownerStruct)
      {
        if(!soe->decl->owner)
          errMsgLoc(expr, "Can't use static subroutine " << soe->decl->name <<
              " in place of member of struct " << callable->ownerStruct->name)
        else if(!callable->ownerStruct)
          errMsgLoc(expr, "Can't use member " << soe->decl->name <<
              " of struct " << soe->decl->owner->name <<
              " in place of static subroutine.")
        else
          errMsgLoc(expr, "Can't use member " << soe->decl->name <<
              " of struct " << soe->decl->owner->name <<
              " in place of member of " << callable->ownerStruct->name)
      }
      else
        errMsgLoc(expr, "No overload of " << soe->decl->name <<
            " has parameters matching " << callable->getName());
    }
    //match successful, create the new expression
    Node* loc = expr;
    if(thisObject)
    {
      //only Subroutine can be a member, not Extern
      Subroutine* subr = dynamic_cast<Subroutine*>(sb);
      INTERNAL_ASSERT(subr);
      expr = new StructMem(thisObject, subr);
    }
    else
      expr = new SubroutineExpr(sb);
    expr->resolve();
    expr->setLocation(loc);
  }
  else if(!typesSame(reqType, expr->type))
  {
    Node* loc = expr;
    //do normal conversion (if allowed)
    expr = new Converted(expr, reqType);
    expr->resolve();
    expr->setLocation(loc);
  }
  //otherwise, nothing to do (types already match)
}

