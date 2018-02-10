#include "Meta.hpp"

#include "TypeSystem.hpp"
#include "Expression.hpp"
#include "Subroutine.hpp"

using namespace TypeSystem;

CallStack callStack;

namespace Meta
{
namespace Interp
{

Value* expression(Expression* expr)
{
  if(UnaryArith* ua = dynamic_cast<UnaryArith*>(expr))
  {
    Value* operand = expression(ua->expr);
    if(ua->op == BNOT)
    {
      PrimitiveValue* pv = new PrimitiveValue(*((PrimitiveValue*) operand));
      if(pv->t == PrimitiveValue::LL)
      {
        pv->data.ll = ~pv->data.ll;
      }
      else if(pv->t == PrimitiveValue::ULL)
      {
        pv->data.ull = ~pv->data.ull;
      }
      return pv;
    }
    else if(ua->op == LNOT)
    {
      PrimitiveValue* pv = new PrimitiveValue(*((PrimitiveValue*) operand));
      pv->data.b = !pv->data.b;
      return pv;
    }
    else if(ua->op == SUB)
    {
      //can't directly negate unsigned value:
      //if Onyx src did this, a conversion to signed type is applied first
      if(pv->t == PrimitiveValue::LL)
      {
        pv->data.ll = -(pv->data.ll);
      }
      else if(pv->t == PrimitiveValue::F)
      {
        pv->data.f = -(pv->data.f);
      }
      else if(pv->t == PrimitiveValue::D)
      {
        pv->data.d = -(pv->data.d);
      }
      else
      {
        INTERNAL_ERROR;
      }
    }
  }
  else if(BinaryArith* ba = dynamic_cast<BinaryArith*>(expr))
  {
    Value* lhs = expression(ba->lhs);
    Value* rhs = expression(ba->rhs);
    switch(ba->op)
    {
      case PLUS:
        {
          //plus is special: can be concat, prepend or append
          if(lhs->type->isArray() && rhs->type->isArray())
          {
            CompoundValue* larray = (CompoundValue*) lhs;
            CompoundValue* rarray = (CompoundValue*) rhs;
            CompoundValue* cat = new CompoundValue(*larray);
            for(auto elem : rarray->data)
            {
              cat->data.push_back(elem);
            }
            return cat;
          }
          else if(lhs->type->isArray())
          {
            CompoundValue* larray = (CompoundValue*) lhs;
            CompoundValue* appended = new CompoundValue(*larray);
            appended->value->data.push_back(rhs);
            return appended;
          }
          else if(rhs->type->isArray())
          {
            CompoundValue* rarray = (CompoundValue*) rhs;
            CompoundValue* prepended = new CompoundValue;
            prepended->data.push_back(lhs);
            for(auto elem : rarray->data)
            {
              prepended->data.push_back(elem);
            }
            return prepended;
          }
          else
          {
            //regular numerical addition
            auto lhsPrim = (PrimitiveValue*) lhs;
            auto rhsPrim = (PrimitiveValue*) rhs;
            PrimitiveValue* pv = new PrimitiveValue(*lhsPrim);
            if(pv->t == PrimitiveValue::LL)
            {
              pv->data.ll = lhsPrim->data.ll + rhsPrim->data.ll;
            }
            else if(pv->t == PrimitiveValue::ULL)
            {
              pv->data.ull = lhsPrim->data.ull + rhsPrim->data.ull;
            }
            else if(pv->t == PrimitiveValue::D)
            {
              pv->data.d = lhsPrim->data.d + rhsPrim->data.d;
            }
            else if(pv->t == PrimitiveValue::F)
            {
              pv->data.f = lhsPrim->data.f + rhsPrim->data.f;
            }
            return pv;
          }
        }
      case SUB:
        {
          auto lhsPrim = (PrimitiveValue*) lhs;
          auto rhsPrim = (PrimitiveValue*) rhs;
          PrimitiveValue* pv = new PrimitiveValue(*lhsPrim);
          if(pv->t == PrimitiveValue::LL)
          {
            pv->data.ll = lhsPrim->data.ll - rhsPrim->data.ll;
          }
          else if(pv->t == PrimitiveValue::ULL)
          {
            pv->data.ull = lhsPrim->data.ull - rhsPrim->data.ull;
          }
          else if(pv->t == PrimitiveValue::D)
          {
            pv->data.d = lhsPrim->data.d - rhsPrim->data.d;
          }
          else if(pv->t == PrimitiveValue::F)
          {
            pv->data.f = lhsPrim->data.f - rhsPrim->data.f;
          }
          return pv;
        }
      case MUL:
        {
          auto lhsPrim = (PrimitiveValue*) lhs;
          auto rhsPrim = (PrimitiveValue*) rhs;
          PrimitiveValue* pv = new PrimitiveValue(*lhsPrim);
          if(pv->t == PrimitiveValue::LL)
          {
            pv->data.ll = lhsPrim->data.ll * rhsPrim->data.ll;
          }
          else if(pv->t == PrimitiveValue::ULL)
          {
            pv->data.ull = lhsPrim->data.ull * rhsPrim->data.ull;
          }
          else if(pv->t == PrimitiveValue::D)
          {
            pv->data.d = lhsPrim->data.d * rhsPrim->data.d;
          }
          else if(pv->t == PrimitiveValue::F)
          {
            pv->data.f = lhsPrim->data.f * rhsPrim->data.f;
          }
          return pv;
        }
      case DIV:
        {
          auto lhsPrim = (PrimitiveValue*) lhs;
          auto rhsPrim = (PrimitiveValue*) rhs;
          PrimitiveValue* pv = new PrimitiveValue(*lhsPrim);
          if(pv->t == PrimitiveValue::LL)
          {
            if(rhsPrim->data.ll == 0)
            {
              ERR_MSG("integer division by 0");
            }
            pv->data.ll = lhsPrim->data.ll / rhsPrim->data.ll;
          }
          else if(pv->t == PrimitiveValue::ULL)
          {
            if(rhsPrim->data.ull == 0)
            {
              ERR_MSG("integer division by 0");
            }
            pv->data.ull = lhsPrim->data.ull / rhsPrim->data.ull;
          }
          else if(pv->t == PrimitiveValue::D)
          {
            if(rhsPrim->data.d == 0)
            {
              ERR_MSG("floating-point division by 0");
            }
            pv->data.d = lhsPrim->data.d / rhsPrim->data.d;
          }
          else if(pv->t == PrimitiveValue::F)
          {
            if(rhsPrim->data.f == 0)
            {
              ERR_MSG("floating-point division by 0");
            }
            pv->data.f = lhsPrim->data.f / rhsPrim->data.f;
          }
          return pv;
        }
      case MOD:
        {
          //mod only works with integers
          //like DIV, rhs of 0 is illegal
          auto lhsPrim = (PrimitiveValue*) lhs;
          auto rhsPrim = (PrimitiveValue*) rhs;
          PrimitiveValue* pv = new PrimitiveValue(*lhsPrim);
          if(pv->t == PrimitiveValue::LL)
          {
            if(rhsPrim->data.ll == 0)
            {
              ERR_MSG("integer mod (remainder) by 0");
            }
            pv->data.ll = lhsPrim->data.ll % rhsPrim->data.ll;
          }
          else if(pv->t == PrimitiveValue::ULL)
          {
            if(rhsPrim->data.ull == 0)
            {
              ERR_MSG("integer mod (remainder) by 0");
            }
            pv->data.ull = lhsPrim->data.ull % rhsPrim->data.ull;
          }
          else
          {
            INTERNAL_ERROR;
          }
          return pv;
        }
      case LOR:
        {
          //operator ||
          auto lhsPrim = (PrimitiveValue*) lhs;
          auto rhsPrim = (PrimitiveValue*) rhs;
          PrimitiveValue* pv = new PrimitiveValue(*lhsPrim);
          pv->data.b = lhsPrim->data.b || rhsPrim->data.b;
          return pv;
        }
      case BOR:
        {
          //operator |
          auto lhsPrim = (PrimitiveValue*) lhs;
          auto rhsPrim = (PrimitiveValue*) rhs;
          PrimitiveValue* pv = new PrimitiveValue(*lhsPrim);
          if(pv->t == PrimitiveValue::LL)
          {
            pv->data.ll = lhsPrim->data.ll | rhsPrim->data.ll;
          }
          else if(pv->t == PrimitiveValue::ULL)
          {
            pv->data.ull = lhsPrim->data.ull | rhsPrim->data.ull;
          }
          else
          {
            INTERNAL_ERROR;
          }
          return pv;
        }
      case BXOR:
        {
          //operator ^
          auto lhsPrim = (PrimitiveValue*) lhs;
          auto rhsPrim = (PrimitiveValue*) rhs;
          PrimitiveValue* pv = new PrimitiveValue(*lhsPrim);
          if(pv->t == PrimitiveValue::LL)
          {
            pv->data.ll = lhsPrim->data.ll ^ rhsPrim->data.ll;
          }
          else if(pv->t == PrimitiveValue::ULL)
          {
            pv->data.ull = lhsPrim->data.ull ^ rhsPrim->data.ull;
          }
          else
          {
            INTERNAL_ERROR;
          }
          return pv;
        }
      case LAND:
        {
          //operator &&
          auto lhsPrim = (PrimitiveValue*) lhs;
          auto rhsPrim = (PrimitiveValue*) rhs;
          PrimitiveValue* pv = new PrimitiveValue(*lhsPrim);
          pv->data.b = lhsPrim->data.b && rhsPrim->data.b;
          return pv;
        }
      case BAND:
        {
          //operator &
          auto lhsPrim = (PrimitiveValue*) lhs;
          auto rhsPrim = (PrimitiveValue*) rhs;
          PrimitiveValue* pv = new PrimitiveValue(*lhsPrim);
          if(pv->t == PrimitiveValue::LL)
          {
            pv->data.ll = lhsPrim->data.ll & rhsPrim->data.ll;
          }
          else if(pv->t == PrimitiveValue::ULL)
          {
            pv->data.ull = lhsPrim->data.ull & rhsPrim->data.ull;
          }
          else
          {
            INTERNAL_ERROR;
          }
          return pv;
        }
      case SHL:
        {
          //operator <<
          auto lhsPrim = (PrimitiveValue*) lhs;
          auto rhsPrim = (PrimitiveValue*) rhs;
          PrimitiveValue* pv = new PrimitiveValue(*lhsPrim);
          if(pv->t == PrimitiveValue::LL)
          {
            if(rhsPrim->data.ll < 0)
            {
              ERR_MSG("integer shifted left by negative value");
            }
            else if(rhsPrim->data.ll >= 8 * rhsPrim->size)
            {
              ERR_MSG("integer shifted left by >= width bits");
            }
            pv->data.ll = lhsPrim->data.ll << rhsPrim->data.ll;
          }
          else if(pv->t == PrimitiveValue::ULL)
          {
            if(rhsPrim->data.ull >= 8 * rhsPrim->size)
            {
              ERR_MSG("integer shifted left by >= width bits");
            }
            pv->data.ull = lhsPrim->data.ull << rhsPrim->data.ull;
          }
          else
          {
            INTERNAL_ERROR;
          }
          return pv;
        }
      case SHR:
        {
          //operator <<
          auto lhsPrim = (PrimitiveValue*) lhs;
          auto rhsPrim = (PrimitiveValue*) rhs;
          PrimitiveValue* pv = new PrimitiveValue(*lhsPrim);
          if(pv->t == PrimitiveValue::LL)
          {
            if(rhsPrim->data.ll < 0)
            {
              ERR_MSG("integer shifted left by negative value");
            }
            else if(rhsPrim->data.ll >= 8 * rhsPrim->size)
            {
              ERR_MSG("integer shifted left by >= width bits");
            }
            pv->data.ll = lhsPrim->data.ll >> rhsPrim->data.ll;
          }
          else if(pv->t == PrimitiveValue::ULL)
          {
            if(rhsPrim->data.ull >= 8 * rhsPrim->size)
            {
              ERR_MSG("integer shifted left by >= width bits");
            }
            pv->data.ull = lhsPrim->data.ull >> rhsPrim->data.ull;
          }
          else
          {
            INTERNAL_ERROR;
          }
          return pv;
        }
      case CMPEQ:
          return compareEqual(lhs, rhs);
      case CMPNEQ:
          return !compareEqual(lhs, rhs);
      case CMPL:
          return compareLess(lhs, rhs);
      case CMPLE:
          return !compareLess(rhs, lhs);
      case CMPG:
          return compareLess(rhs, lhs);
      case CMPGE:
          return !compareLess(lhs, rhs);
      default:
        INTERNAL_ERROR;
    }
  }
  else if(IntLiteral* il = dynamic_cast<IntLiteral*>(expr))
  {
    //create a new unsigned integer primitive to hold il's value
    auto pv = new PrimitiveValue;
    pv->data.ull = il->value;
    pv->t = PrimitiveValue::ULL;
    //make sure type is exactly il's type
    pv->size = ((IntegerType*) il->type)->size;
    return pv;
  }
  else if(FloatLiteral* fl = dynamic_cast<FloatLiteral*>(expr))
  {
    //create a new unsigned integer primitive to hold il's value
    auto pv = new PrimitiveValue;
    pv->data.d = fl->value;
    pv->t = PrimitiveValue::d;
    return pv;
  }
  else if(StringLiteral* sl = dynamic_cast<StringLiteral*>(expr))
  {
    auto av = new CompoundValue;
    for(auto ch : sl->value)
    {
      auto elem = new PrimitiveValue;
      elem->t = PrimitiveValue::ULL;
      elem->size = 1;
      elem->data.ull = ch;
      av->data.push_back(elem);
    }
    return av;
  }
  else if(CharLiteral* cl = dynamic_cast<CharLiteral*>(expr))
  {
    //encode cl's value as a 1-byte unsigned integer
    auto pv = new PrimitiveValue;
    pv->data.ull = cl->value;
    pv->t = PrimitiveValue::ULL;
    pv->size = 1;
    return pv;
  }
  else if(BoolLiteral* bl = dynamic_cast<BoolLiteral*>(expr))
  {
    auto pv = new PrimitiveValue;
    pv->data.b = bl->value;
    pv->t = PrimitiveValue::B;
    return pv;
  }
  else if(CompoundLiteral* cl = dynamic_cast<CompoundLiteral*>(expr))
  {
    //only 2 cases: tuple -> compound value, array -> array value
    if(cl->type->isTuple() || cl->type->isArray())
    {
      auto cv = new CompoundValue;
      for(auto mem : cl->members)
      {
        cv->data.push_back(expression(mem));
      }
      return cv;
    }
  }
  else if(Indexed* in = dynamic_cast<Indexed*>(expr))
  {
    Value* group = expression(in->group);
    Value* index = expression(in->index);
    if(group->type->isMap())
    {
      //map lookup
      auto mv = (MapValue*) group;
      auto mt = (MapType*) group->type;
      //lookup in the map produces either a value, or "not found"
      auto it = mv->data.find(index);
      auto uv = new UnionValue;
      if(it == mv->data.end())
      {
        auto errVal = new PrimitiveValue;
        errVal->t = PrimitiveValue::ERR;
        uv->actual = primitives[Prim::ERROR];
        uv->v = errVal;
      }
      else
      {
        uv->actual = mt->value;
        uv->v = it->second;
      }
      return uv;
    }
    else
    {
      //index must be integer
      long long index = 0;
      auto primIndex = (PrimitiveValue*) index;
      if(primIndex->t == PrimitiveValue::ULL)
      {
        index = primIndex->data.ull;
      }
      else if(primIndex->t == PrimitiveValue::LL)
      {
        index = primIndex->data.ll;
      }
      else
      {
        INTERNAL_ERROR;
      }
      auto compoundVal = (CompoundValue*) group;
      if(index < 0 || index >= compoundVal->members.size())
      {
        ERR_MSG("array/tuple index out of bounds");
      }
      return compoundVal->data[index];
    }
  }
  else if(CallExpr* ce = dynamic_cast<CallExpr*>(expr))
  {
    return call(ce);
  }
  else if(VarExpr* ve = dynamic_cast<VarExpr*>(expr))
  {
    //lazily create (0-initialize) the variable's value if it doesn't exist
    if(varValues.find(ve->var) == varValues.end())
    {
      auto newVal = initialize(ve->type);
      varValues[ve->var] = newVal;
      return newVal;
    }
    else
    {
      return varValues[ve->var];
    }
  }
  else if(NewArray* na = dynamic_cast<NewArray*>(expr))
  {
    //allocate rectangular array with proper dimensions, then
    //initialize each element
    auto elemType = ((ArrayType*) na->type)->elem;
    CompoundValue* root = new CompoundRoot;
    vector<unsigned> sizes;
    for(size_t i = 0; i < na->dims.size(); i++)
    {
      auto dimPrim = (PrimitiveValue*) expression(na->dims[i]);
      if(dimPrim->t == PrimitiveValue::ULL)
      {
        sizes.push_back(dimPrim->data.ull);
      }
      else if(dimPrim->t == PrimitiveValue::LL)
      {
        sizes.push_back(dimPrim->data.ll);
      }
      else
      {
        INTERNAL_ERROR;
      }
    }
    //for each array "depth" levels from root, create subarrays or elements
    auto fillLevel = [&] (CompoundValue* cv, int depth)
    {
      for(size_t i = 0; i < sizes[depth]; i++)
      {
        if(depth == sizes.size() - 1)
        {
          //base level: initialize single elements
          cv->data.push_back(initialize(elemType));
        }
        else
        {
          auto subArray = new CompoundValue;
          cv->data.push_back(subArray);
          fillLevel(subArray, depth + 1);
        }
      }
    };
    fillLevel(root, 0);
  }
  else if(dynamic_cast<ErrorVal*>(expr))
  {
    auto pv = new PrimitiveValue;
    pv->t = PrimitiveValue::ERR;
    return pv;
  }
  else
  {
    INTERNAL_ERROR;
  }
}

Value* call(CallExpr* c)
{
  //eval args and "this" object (if applicable)
  auto callable = (CallableValue*) expression(c->callable);
  callStack.push();
  //compute "this" if callable is a struct member
  if(auto structMem = dynamic_cast<StructMem*>(callable))
  {
    callStack.top().thisVal = expression(structMem->base);
  }
  for(size_t i = 0; i < callable->subr->args; i++)
  {
    callStack.top().vars[callable->subr->args[i]]
      = expression(callable->args[i]);
  }
  //execute the body
  statement(callable->subr->body);
  //retrieve the return value before destroying callee frame
  Value* returned = callStack.top().returnVal;
  callStack.pop();
  return returned;
}

Value* initialize(TypeSystem::Type* type)
{
  if(type->isPrimitive())
  {
    auto pv = new PrimitiveValue;
    pv->type = type;
    if(auto intType = dynamic_cast<IntType*>(type))
    {
      pv->size = intType->size;
      if(intType->isSigned)
      {
        pv->t = PrimitiveValue::LL;
        pv->data.ll = 0;
      }
      else
      {
        pv->t = PrimitiveValue::ULL;
        pv->data.ull = 0;
      }
    }
    else if(auto floatType = dynamic_cast<FloatType*>(type))
    {
      if(floatType->size == 4)
      {
        pv->t = PrimitiveValue::F;
        pv->data.f = 0;
      }
      else
      {
        pv->t = PrimitiveValue::D;
        pv->data.d = 0;
      }
    }
    else if(dynamic_cast<BoolType*>(type))
    {
      pv->t = PrimitiveValue::B;
      pv->data.b = false;
    }
    else if(dynamic_cast<CharType*>(type))
    {
      pv->t = PrimitiveValue::LL;
      pv->data.ll = 0;
    }
    else
    {
      INTERNAL_ERROR;
    }
    return pv;
  }
  else if(type->isStruct())
  {
    StructType* st = dynamic_cast<StructType*>(type);
    CompoundValue* cv = new CompoundValue;
    for(auto mem : st->members)
    {
      cv->data.push_back(initialize(mem->type));
    }
    return cv;
  }
  else if(type->isTuple())
  {
    TupleType* tt = (TupleType*) type;
    CompoundValue* cv = new CompoundValue;
    for(auto mem : tt->members)
    {
      cv->data.push_back(initialize(mem));
    }
    return cv;
  }
  else if(type->isArray())
  {
    return new CompoundValue;
  }
  else if(type->isMap())
  {
    return new MapType;
  }
  else if(type->isUnion())
  {
    UnionType* ut = (UnionType*) type;
    UnionValue* uv = new UnionValue;
    uv->actual = ut->options[0];
    uv->v = initialize(ut->options[0]);
    return uv;
  }
  else if(type->isCallable())
  {
    return new CallableValue;
  }
}

bool compareEqual(Value* lhs, Value* rhs)
{
  //note: lhs and rhs must have exactly the same type
  if(lhs->type != rhs->type)
  {
    INTERNAL_ERROR;
  }
  if(lhs->type->isPrimitive())
  {
    PrimitiveValue* lhsPrim = dynamic_cast<PrimitiveValue*>(lhs);
    PrimitiveValue* rhsPrim = dynamic_cast<PrimitiveValue*>(rhs);
    if(lhsPrim->t == PrimitiveValue::ULL)
    {
      return lhsPrim->data.ull == rhsPrim->data.ull;
    }
    else if(lhsPrim->t == PrimitiveValue::LL)
    {
      return lhsPrim->data.ll == rhsPrim->data.ll;
    }
    else if(lhsPrim->t == PrimitiveValue::F)
    {
      return lhsPrim->data.f == rhsPrim->data.f;
    }
    else if(lhsPrim->t == PrimitiveValue::D)
    {
      return lhsPrim->data.d == rhsPrim->data.d;
    }
    else if(lhsPrim->t == PrimitiveValue::B)
    {
      return lhsPrim->data.b == rhsPrim->data.b;
    }
    else
    {
      INTERNAL_ERROR;
    }
  }
  else if(lhs->type->isStruct() || lhs->type->isTuple())
  {
    //lex compare members
    CompoundValue* lhsCompound = dynamic_cast<CompoundValue*>(lhs);
    CompoundValue* rhsCompound = dynamic_cast<CompoundValue*>(rhs);
    if(lhsCompound->members.size() != rhsCompound->members.size())
    {
      INTERNAL_ERROR;
    }
    for(size_t i = 0; i < lhsCompound->members.size(); i++)
    {
      if(!compareEqual(lhsCompound->members[i], rhsCompound->members[i]))
      {
        return false;
      }
    }
    return true;
  }
  else if(lhs->type->isArray())
  {
    CompoundValue* lhsArray = dynamic_cast<CompoundValue*>(lhs);
    CompoundValue* rhsArray = dynamic_cast<CompoundValue*>(rhs);
    if(lhsArray->data.size() != rhsArray->data.size())
    {
      return false;
    }
    for(size_t i = 0; i < lhsArray->data.size(); i++)
    {
      if(!compareEqual(lhsArray->data[i], rhsArray->data[i]))
      {
        return false;
      }
    }
    return true;
  }
  else if(lhs->type->isUnion())
  {
    UnionValue* lhsUnion = dynamic_cast<UnionValue*>(lhs);
    UnionValue* rhsUnion = dynamic_cast<UnionValue*>(rhs);
    if(lhsUnion->actual != rhsUnion->actual)
    {
      return false;
    }
    return compareEqual(lhsUnion->v, rhsUnion->v);
  }
  else if(lhs->type->isMap())
  {
    //key -> value mapping is always surjective, so just test that for each
    //lhs k-v, make sure rhs has same k and that k maps to v
    MapValue* lhsMap = dynamic_cast<MapValue*>(lhs);
    MapValue* rhsMap = dynamic_cast<MapValue*>(rhs);
    if(lhsMap->data.size() != rhsMap->data.size())
    {
      return false;
    }
    for(auto& kv : lhsMap->data)
    {
      auto rhsIter = rhsMap->data.find(kv.first);
      if(rhsIter == rhsMap->end())
      {
        return false;
      }
    }
  }
  else
  {
    INTERNAL_ERROR;
  }
}

bool compareLess(Value* lhs, Value* rhs)
{
  if(lhs->type != rhs->type)
  {
    INTERNAL_ERROR;
  }
  if(lhs->type->isPrimitive())
  {
    PrimitiveValue* lhsPrim = dynamic_cast<PrimitiveValue*>(lhs);
    PrimitiveValue* rhsPrim = dynamic_cast<PrimitiveValue*>(rhs);
    if(lhsPrim->t == PrimitiveValue::ULL)
    {
      return lhsPrim->data.ull < rhsPrim->data.ull;
    }
    else if(lhsPrim->t == PrimitiveValue::LL)
    {
      switch(lhsPrim->size)
      {
        case 1:
          return ((int8_t) lhsPrim->data.ll) < ((int8_t) rhsPrim->data.ll);
        case 2:
          return ((int16_t) lhsPrim->data.ll) < ((int16_t) rhsPrim->data.ll);
        case 4:
          return ((int32_t) lhsPrim->data.ll) < ((int32_t) rhsPrim->data.ll);
        case 8:
          return ((int64_t) lhsPrim->data.ll) < ((int64_t) rhsPrim->data.ll);
        default:
          INTERNAL_ERROR;
      }
    }
    else if(lhsPrim->t == PrimitiveValue::F)
    {
      return lhsPrim->data.f < rhsPrim->data.f;
    }
    else if(lhsPrim->t == PrimitiveValue::D)
    {
      return lhsPrim->data.d < rhsPrim->data.d;
    }
    else if(lhsPrim->t == PrimitiveValue::B)
    {
      return !lhsPrim->data.b && rhsPrim->data.b;
    }
    else
    {
      INTERNAL_ERROR;
    }
  }
  else if(lhs->type->isStruct() || lhs->type->isTuple())
  {
    //lex compare members
    CompoundValue* lhsCompound = dynamic_cast<CompoundValue*>(lhs);
    CompoundValue* rhsCompound = dynamic_cast<CompoundValue*>(rhs);
    if(lhsCompound->members.size() != rhsCompound->members.size())
    {
      INTERNAL_ERROR;
    }
    for(size_t i = 0; i < lhsCompound->members.size(); i++)
    {
      if(compareLess(lhsCompound->members[i], rhsCompound->members[i]))
      {
        return true;
      }
      else if(!compareEqual(lhsCompound->members[i], rhsCompound->members[i]))
      {
        //element greater
        return false;
      }
    }
    //equal
    return false;
  }
  else if(lhs->type->isArray())
  {
    CompoundValue* lhsArray = dynamic_cast<CompoundValue*>(lhs);
    CompoundValue* rhsArray = dynamic_cast<CompoundValue*>(rhs);
    for(size_t i = 0;
        i < std::min(lhsArray->data.size(), rhsArray->data.size());
        i++)
    {
      if(compareLess(lhsArray->data[i], rhsArray->data[i]))
      {
        return true;
      }
      if(!compareEqual(lhsArray->data[i], rhsArray->data[i]))
      {
        return false;
      }
    }
    //all elements up to here have been equal: the smaller array is "less" now
    return lhsArray->data.size() < rhsArray->data.size();
  }
  else if(lhs->type->isUnion())
  {
    UnionValue* lhsUnion = dynamic_cast<UnionValue*>(lhs);
    UnionValue* rhsUnion = dynamic_cast<UnionValue*>(rhs);
    UnionType* ut = dynamic_cast<UnionType*>(lhs->type);
    //get the relative order of tags within union type's option list
    int lhsTagIndex = -1;
    int rhsTagIndex = -1;
    for(size_t i = 0; i < lhsUnion->type->options.size(); i++)
    {
      if(ut->options[i] == lhsUnion->actual)
        lhsTagIndex = i;
      if(ut->options[i] == rhsUnion->actual)
        rhsTagIndex = i;
    }
    if(lhsTagIndex < 0 || rhsTagIndex < 0)
    {
      INTERNAL_ERROR;
    }
    if(lhsTagIndex < rhsTagIndex)
      return true;
    if(rhsTagIndex > rhsTagIndex)
      return false;
    //underlying types equal, can compare the values
    return compareLess(lhsUnion->v, rhsUnion->v);
  }
  else if(lhs->type->isMap())
  {
    //key -> value mapping is always surjective, so just test that for each
    //lhs k-v, make sure rhs has same k and that k maps to v
    MapValue* lhsMap = dynamic_cast<MapValue*>(lhs);
    MapValue* rhsMap = dynamic_cast<MapValue*>(rhs);
    //generate sorted lists of each maps' keys
    vector<Value*> lhsKeys;
    for(auto& kv : lhsMap->data)
    {
      lhsKeys.push_back(kv.first);
    }
    std::sort(lhsKeys.begin(), lhsKeys.end(), compareLess);
    vector<Value*> rhsKeys;
    for(auto& kv : rhsMap->data)
    {
      rhsKeys.push_back(kv.first);
    }
    std::sort(rhsKeys.begin(), rhsKeys.end(), compareLess);
    //lex compare the key arrays
    for(size_t i = 0; i < std::min(lhsKeys.size(), rhsKeys.size()), i++)
    {
      if(compareLess(lhsKeys[i], rhsKeys[i]))
      {
        return true;
      }
      else if(!compareEqual(lhsKeys[i], rhsKeys[i]))
      {
        return false;
      }
    }
    if(lhsKeys.size() < rhsKeys.size())
    {
      return true;
    }
    else if(lhsKeys.size() > rhsKeys.size())
    {
      return false;
    }
    //both maps have exactly the same set of keys, so now lex compare values
    size_t numKeys = lhsKeys.size();
    for(size_t i = 0; i < numKeys; i++)
    {
      Value* key = lhsKeys[i];
      if(compareLess(lhsMap->data.find(key)->second, rhsMap->Data.find(key)->second))
      {
        return true;
      }
      else if(!compareEqual(lhsMap->data.find(key)->second, rhsMap->Data.find(key)->second))
      {
        return false;
      }
    }
    //maps identical, so lhs < rhs false
    return false;
  }
  else
  {
    INTERNAL_ERROR;
  }
}

Value* convert(Value* val, TypeSystem::Type* type)
{
  if(!type->canConvert(val->type))
  {
    INTERNAL_ERROR;
  }
  //if types are exactly equal, don't need to do anything
  if(val->type == type)
  {
    return val;
  }
  //All supported type conversions:
  //  -All primitives can be converted to each other trivially
  //    -floats/doubles truncated to integer as in C
  //    -ints converted to each other as in C
  //    -char treated as integer
  //    -any number converted to bool with nonzero being true
  //  -Out = struct: in = struct or tuple
  //  -Out = tuple: in = struct or tuple
  //  -Out = array: in = struct, tuple or array
  //  -Out = map: in = map, array, or tuple
  //    -in = map: convert keys to keys and values to values;
  //      since maps are unordered, key conflicts are UB
  //    -in = array/tuple: key is int, values converted to values
  //    -in = struct: key is string, value 
  //conversion from one primitive to another is same semantics as C
  if(auto intType = dynamic_cast<IntegerType*>(type))
  {
    PrimitiveValue* in = (PrimitiveValue*) val;
    PrimitiveValue* out = new PrimitiveValue;
    out->type = type;
    if(intType->isSigned)
    {
      out->t = PrimitiveValue::LL;
      switch(in->t)
      {
        case PrimitiveValue::LL:
          out->data.ll = in->data.ll;
          break;
        case PrimitiveValue::ULL:
          out->data.ll = in->data.ull;
          break;
        case PrimitiveValue::F:
          out->data.ll = in->data.f;
          break;
        case PrimitiveValue::D:
          out->data.ll = in->data.d;
          break;
        default:;
      }
      out->size = intType->size;
    }
    else
    {
      out->t = PrimitiveValue::ULL;
      switch(in->t)
      {
        case PrimitiveValue::LL:
          out->data.ull = in->data.ll;
          break;
        case PrimitiveValue::ULL:
          out->data.ull = in->data.ull;
          break;
        case PrimitiveValue::F:
          out->data.ull = in->data.f;
          break;
        case PrimitiveValue::D:
          out->data.ull = in->data.d;
          break;
        default:;
      }
      out->size = intType->size;
    }
    return out;
  }
  else if(auto floatType = dynamic_cast<FloatType*>(type))
  {
    PrimitiveValue* in = (PrimitiveValue*) val;
    PrimitiveValue* out = new PrimitiveValue;
    out->type = type;
    if(floatType->size == 4)
    {
      out->t = PrimitiveValue::F;
      switch(in->t)
      {
        case PrimitiveValue::LL:
          out->data.f = in->data.ll;
          break;
        case PrimitiveValue::ULL:
          out->data.f = in->data.ull;
          break;
        case PrimitiveValue::F:
          out->data.f = in->data.f;
          break;
        case PrimitiveValue::D:
          out->data.f = in->data.d;
          break;
        default:;
      }
    }
    else
    {
      out->t = PrimitiveValue::D;
      switch(in->t)
      {
        case PrimitiveValue::LL:
          out->data.d = in->data.ll;
          break;
        case PrimitiveValue::ULL:
          out->data.d = in->data.ull;
          break;
        case PrimitiveValue::F:
          out->data.d = in->data.f;
          break;
        case PrimitiveValue::D:
          out->data.d = in->data.d;
          break;
        default:;
      }
    }
    return out;
  }
  else if(dynamic_cast<BoolType*>(type))
  {
    PrimitiveValue* in = (PrimitiveValue*) val;
    PrimitiveValue* out = new PrimitiveValue;
    out->type = type;
    out->t = PrimitiveValue::D;
    switch(in->t)
    {
      case PrimitiveValue::LL:
        out->data.b = in->data.ll != 0;
        break;
      case PrimitiveValue::ULL:
        out->data.b = in->data.ull != 0;
        break;
      case PrimitiveValue::F:
        out->data.b = in->data.f != 0;
        break;
      case PrimitiveValue::D:
        out->data.b = in->data.d != 0;
        break;
      default:;
    }
    return out;
  }
  else if(dynamic_cast<CharType*>(type))
  {
    PrimitiveValue* in = (PrimitiveValue*) val;
    PrimitiveValue* out = new PrimitiveValue;
    out->type = type;
    out->t = PrimitiveValue::LL;
    switch(in->t)
    {
      case PrimitiveValue::LL:
        out->data.ll = in->data.ll;
        break;
      case PrimitiveValue::ULL:
        out->data.ll = in->data.ull;
        break;
      case PrimitiveValue::F:
        out->data.ll = in->data.f;
        break;
      case PrimitiveValue::D:
        out->data.ll = in->data.d;
        break;
      default:;
    }
    return out;
  }
  else if(auto outStruct = dynamic_cast<StructType*>(type))
  {
    //tuples and structs may convert to struct
    CompoundValue* in = (CompoundValue*) val;
    CompoundValue* out = new CompoundValue;
    out->type = type;
    for(size_t i = 0; i < inStruct->members.size(); i++)
    {
      out->data.push_back(convert(in->data[i],
            outStruct->members[i]->type));
    }
    return out;
  }
  else if(auto outTuple = dynamic_cast<TupleType*>(type))
  {
    //tuples and structs may convert to struct
    CompoundValue* in = (CompoundValue*) val;
    CompoundValue* out = new CompoundValue;
    out->type = type;
    for(size_t i = 0; i < inStruct->members.size(); i++)
    {
      out->data.push_back(convert(in->data[i], outTuple->members[i]));
    }
    return out;
  }
  else if(auto outArray = dynamic_cast<ArrayType*>(type))
  {
    CompoundValue* in = (CompoundValue*) val;
    CompoundValue* out = new CompoundValue;
    out->type = type;
    Type* subtype = outArray->subtype;
    for(auto elem : in->data)
    {
      out->data.push_back(convert(elem, subtype));
    }
    return out;
  }
  else if(auto outMap = dynamic_cast<MapType*>(type))
  {
    MapValue* out = new MapValue;
    Type* key = outMap->key;
    Type* value = outMap->value;
    out->type = type;
    if(auto inMap = dynamic_cast<MapValue*>(val))
    {
      for(auto& kv : inMap->data)
      {
        out->data[convert(kv->first, key)] = convert(kv->second, value);
      }
    }
    else if(auto inCompound = dynamic_cast<CompoundValue*>(val))
    {
      for(size_t i = 0; i < inCompound->data.size(); i++)
      {
        out->data[expression(new IntLiteral(i))]
          = convert(kv->second, value);
      }
    }
    return out;
  }
}

void statement(Statement* stmt)
{
  if(Block* block = dynamic_cast<Block*>(stmt))
  {
  }
  else if(Assign* assign = dynamic_cast<Assign*>(stmt))
  {
  }
  else if(CallStmt* c = dynamic_cast<CallStmt*>(stmt))
  {
    call(c->eval);
  }
  else if(For* f = dynamic_cast<For*>(stmt))
  {
  }
  else if(While* w = dynamic_cast<While*>(stmt))
  {
  }
  else if(If* ifstmt = dynamic_cast<If*>(stmt))
  {
  }
  else if(IfElse* ifelse = dynamic_cast<IfElse*>(stmt))
  {
  }
  else if(Return* ret = dynamic_cast<Return*>(stmt))
  {
  }
  else if(Break* brk = dynamic_cast<Break*>(stmt))
  {
  }
  else if(Contine* cont = dynamic_cast<Continue*>(stmt))
  {
  }
  else if(Print* pr = dynamic_cast<Print*>(stmt))
  {
    for(auto expr : pr->exprs)
    {
      print(expr);
    }
  }
  else if(Assertion* as = dynamic_cast<Assertion*>(stmt))
  {
  }
  else if(Switch* sw = dynamic_cast<Switch*>(stmt))
  {
  }
  else if(Match* ma = dynamic_cast<Match*>(stmt))
  {
  }
  else
  {
    INTERNAL_ERROR;
  }
}

void print(Value* v)
{
  if(auto pv = dynamic_cast<PrimitiveValue*>(v))
  {
    if(auto intType = dynamic_cast<IntType*>(v->type))
    {
      if(pv->t == PrimitiveValue::LL)
      {
        cout << pv->data.ll;
      }
      else if(pv->t == PrimitiveValue::ULL)
      {
        cout << pv->data.ull;
      }
    }
    else if(auto floatType = dynamic_cast<FloatType*>(v->type))
    {
      if(pv->t == PrimitiveValue::F)
      {
        cout << pv->data.f;
      }
      else if(pv->t == PrimitiveValue::D)
      {
        cout << pv->data.d;
      }
    }
    else if(dynamic_cast<CharType*>(v->type))
    {
      cout << (char) pv->data.ull;
    }
    else if(dynamic_cast<BoolType*>(v->type))
    {
      if(pv->data.b)
      {
        cout << "true";
      }
      else
      {
        cout << "false";
      }
    }
    else if(dynamic_cast<ErrorType*>(v->type))
    {
      cout << "error";
    }
    else
    {
      INTERNAL_ERROR;
    }
  }
  else if(auto cv = dynamic_cast<CompoundType*>(v))
  {
    //can be tuple, array or struct
    //all very similar except for punctuation, and name of struct before the list
    char term = 0;
    if(auto structType = dynamic_cast<StructType*>(v->type))
    {
      cout << structType->name << '{';
      term = '}';
    }
    else if(auto tupleType = dynamic_cast<TupleType*>(v->type))
    {
      cout << '(';
      term = ')';
    }
    else if(auto arrayType = dynamic_cast<ArrayType*>(v->type))
    {
      cout << '[';
      term = ']';
    }
    for(size_t i = 0; i < cv->data.size(); i++)
    {
      print(cv->data[i]);
      if(i != cv->data.size() - 1)
      {
        cout << ", ";
      }
    }
    //end the list
    cout << term;
  }
  else if(auto mv = dynamic_cast<MapValue*>(v))
  {
    cout << '{';
    size_t count = 0;
    for(auto& kv : mv->data)
    {
      print(kv->first);
      cout << ": ";
      print(kv->second);
      if(count < kv.size() - 1)
      {
        cout << ", ";
      }
      count++;
    }
    cout << '}';
  }
  else
  {
    INTERNAL_ERROR;
  }
}

}}  //namespace Meta::Interp

