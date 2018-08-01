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

void UnaryArith::resolveImpl(bool final)
{
  resolveExpr(expr, final);
  if(expr->resolved)
  {
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
}

set<Variable*> UnaryArith::getReads()
{
  return expr->getReads();
}

/***************
 * BinaryArith *
 ***************/

BinaryArith::BinaryArith(Expression* l, int o, Expression* r) : op(o), lhs(l), rhs(r) {}

void BinaryArith::resolveImpl(bool final)
{
  resolveExpr(lhs, final);
  resolveExpr(rhs, final);
  if(!lhs->resolved || !rhs->resolved)
  {
    return;
  }
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

/*******************
 * CompoundLiteral *
 *******************/

CompoundLiteral::CompoundLiteral(vector<Expression*>& mems)
  : members(mems) {}

void CompoundLiteral::resolveImpl(bool final)
{
  //first, try to resolve all members
  bool allResolved = true;
  lvalue = true;
  for(size_t i = 0; i < members.size(); i++)
  {
    resolveExpr(members[i], final);
    if(!members[i]->resolved)
    {
      allResolved = false;
      break;
    }
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

/***********
 * Indexed *
 ***********/

Indexed::Indexed(Expression* grp, Expression* ind)
  : group(grp), index(ind) {}

void Indexed::resolveImpl(bool final)
{
  resolveExpr(group, final);
  resolveExpr(index, final);
  if(!group->resolved || !index->resolved)
  {
    return;
  }
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

void CallExpr::resolveImpl(bool final)
{
  resolveExpr(callable, final);
  if(!callable->resolved)
    return;
  auto callableType = dynamic_cast<CallableType*>(callable->type);
  if(!callableType)
  {
    errMsgLoc(this, "attempt to call non-callable expression");
  }
  type = callableType->returnType;
  bool allResolved = callable->resolved;
  for(size_t i = 0; i < args.size(); i++)
  {
    resolveExpr(args[i], final);
    allResolved = allResolved && args[i]->resolved;
  }
  if(!allResolved)
    return;
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

/***********
 * VarExpr *
 ***********/

VarExpr::VarExpr(Variable* v, Scope* s) : var(v), scope(s) {}
VarExpr::VarExpr(Variable* v) : var(v), scope(nullptr) {}

void VarExpr::resolveImpl(bool final)
{
  if(!var->resolved)
    var->resolve(final);
  if(!var->resolved)
  {
    return;
  }
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

/******************
 * SubroutineExpr *
 ******************/

SubroutineExpr::SubroutineExpr(Subroutine* s)
{
  thisObject = nullptr;
  subr = s;
  exSubr = nullptr;
}

SubroutineExpr::SubroutineExpr(Expression* root, Subroutine* s)
{
  thisObject = root;
  subr = s;
  exSubr = nullptr;
}

SubroutineExpr::SubroutineExpr(ExternalSubroutine* es)
{
  thisObject = nullptr;
  subr = nullptr;
  exSubr = es;
}

void SubroutineExpr::resolveImpl(bool final)
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

void StructMem::resolveImpl(bool final)
{
  resolveExpr(base, final);
  if(!base->resolved)
  {
    return;
  }
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

/************
 * NewArray *
 ************/

NewArray::NewArray(Type* elemType, vector<Expression*> dimensions)
{
  elem = elemType;
  dims = dimensions;
}

void NewArray::resolveImpl(bool final)
{
  resolveType(elem, final);
  if(!elem->resolved)
    return;
  for(size_t i = 0; i < dims.size(); i++)
  {
    resolveExpr(dims[i], final);
    if(!dims[i]->resolved)
    {
      return;
    }
    if(!dims[i]->type->isInteger())
    {
      errMsgLoc(dims[i], "array dimensions must be integers");
    }
  }
  type = getArrayType(elem, dims.size());
  resolved = true;
}

/***************
 * ArrayLength *
 ***************/

ArrayLength::ArrayLength(Expression* arr)
{
  array = arr;
}

void ArrayLength::resolveImpl(bool final)
{
  resolveExpr(array, final);
  if(!array->resolved)
  {
    return;
  }
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

void IsExpr::resolveImpl(bool final)
{
  resolveExpr(base, final);
  if(!base->resolved)
    return;
  resolveType(option, final);
  if(!option->resolved)
    return;
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

void AsExpr::resolveImpl(bool final)
{
  resolveExpr(base, final);
  if(!base->resolved)
    return;
  resolveType(option, final);
  if(!option->resolved)
    return;
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

/************
 * EnumExpr *
 ************/

EnumExpr::EnumExpr(EnumConstant* ec)
{
  type = ec->et;
  value = ec;
  resolved = true;
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

void resolveExpr(Expression*& expr, bool final)
{
  if(expr->resolved)
  {
    return;
  }
  auto unres = dynamic_cast<UnresolvedExpr*>(expr);
  if(!unres)
  {
    expr->resolve(final);
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
        if(!final)
        {
          //can't continue, but not an error either
          //(maybe the name just hasn't been declared yet)
          return;
        }
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
              subrThis->resolve(true);
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
              base = new SubroutineExpr(subr);
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
  base->resolve(final);
  //base must be resolved (need its type) to continue
  if(!base->resolved)
    return;
  //look up members in searchScope until a new expr can be formed
  while(nameIter < names.size())
  {
    if(base->type->isArray() && names[nameIter] == "len")
    {
      base = new ArrayLength(base);
      //this resolution can't fail
      base->resolve(true);
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
        if(!final)
        {
          //can't continue, but not an error either
          return;
        }
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
          base = new SubroutineExpr((Subroutine*) found.item);
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
    base->resolve(final);
  }
  //save lexical location of original parsed expression
  base->setLocation(expr);
  expr = base;
}

ostream& operator<<(ostream& os, Expression* e)
{
  INTERNAL_ASSERT(e->resolved);
  if(UnaryArith* ua = dynamic_cast<UnaryArith*>(e))
  {
    os << operatorTable[ua->op] << '(' << ua->expr << ')';
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
    os << uc->value->type->getName() << ' ' << uc->value;
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

