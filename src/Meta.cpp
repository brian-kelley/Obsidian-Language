#include "Meta.hpp"

#include "TypeSystem.hpp"
#include "Expression.hpp"
#include "Subroutine.hpp"

using namespace TypeSystem;

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
        //compute mask that keeps value inside its width
        unsigned long long mask = (1 << (8 * pv->size)) - 1;
        if(pv->t == PrimitiveValue::LL)
        {
          pv->data.ll = (~pv->data.ll) & mask;
        }
        else if(pv->t == PrimitiveValue::ULL)
        {
          pv->data.ull = (~pv->data.ull) & mask;
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
          unsigned long long mask = (1 << (8 * pv->size)) - 1;
          pv->data.ll &= mask;
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
              ArrayValue* larray = (ArrayValue*) lhs;
              ArrayValue* rarray = (ArrayValue*) rhs;
              ArrayValue* cat = new ArrayValue(*larray);
              for(auto elem : rarray->data)
              {
                cat->data.push_back(elem);
              }
              return cat;
            }
            else if(lhs->type->isArray())
            {
              ArrayValue* larray = (ArrayValue*) lhs;
              ArrayValue* appended = new ArrayValue(*larray);
              appended->value->data.push_back(rhs);
              return appended;
            }
            else if(rhs->type->isArray())
            {
              ArrayValue* rarray = (ArrayValue*) rhs;
              ArrayValue* prepended = new ArrayValue;
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
                unsigned long long mask = (1 << (8 * pv->size)) - 1;
                pv->data.ll = lhsPrim->data.ll + rhsPrim->data.ll;
              }
              else if(pv->t == PrimitiveValue::ULL)
              {
                unsigned long long mask = (1 << (8 * pv->size)) - 1;
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
              unsigned long long mask = (1 << (8 * pv->size)) - 1;
              pv->data.ll = lhsPrim->data.ll - rhsPrim->data.ll;
            }
            else if(pv->t == PrimitiveValue::ULL)
            {
              unsigned long long mask = (1 << (8 * pv->size)) - 1;
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
              unsigned long long mask = (1 << (8 * pv->size)) - 1;
              pv->data.ll = (lhsPrim->data.ll * rhsPrim->data.ll) & mask;
            }
            else if(pv->t == PrimitiveValue::ULL)
            {
              unsigned long long mask = (1 << (8 * pv->size)) - 1;
              pv->data.ull = (lhsPrim->data.ull * rhsPrim->data.ull) & mask;
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
            //note that division can only make integers smaller,
            //so there is no need for masking the result as with add/mult
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
        case CMPNEQ:
        case CMPL:
        case CMPLE:
        case CMPG:
        case CMPGE:
        default:
          INTERNAL_ERROR;
      }
    }
    else if(IntLiteral* il = dynamic_cast<IntLiteral*>(expr))
    {
    }
    else if(FloatLiteral* fl = dynamic_cast<FloatLiteral*>(expr))
    {
    }
    else if(StringLiteral* sl = dynamic_cast<StringLiteral*>(expr))
    {
    }
    else if(CharLiteral* cl = dynamic_cast<CharLiteral*>(expr))
    {
    }
    else if(BoolLiteral* bl = dynamic_cast<BoolLiteral*>(expr))
    {
    }
    else if(CompoundLiteral* cl = dynamic_cast<CompoundLiteral*>(expr))
    {
    }
    else if(Indexed* in = dynamic_cast<Indexed*>(expr))
    {
    }
    else if(CallExpr* ce = dynamic_cast<CallExpr*>(expr))
    {
    }
    else if(VarExpr* ve = dynamic_cast<VarExpr*>(expr))
    {
    }
    else if(NewArray* na = dynamic_cast<NewArray*>(expr))
    {
    }
    else
    {
      INTERNAL_ERROR;
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
    else if(lhs->type->isArray())
    {
      ArrayValue* lhsArray = dynamic_cast<ArrayValue*>(lhs);
      ArrayValue* rhsArray = dynamic_cast<ArrayValue*>(rhs);
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
    else if(lhs->type->isArray())
    {
      ArrayValue* lhsArray = dynamic_cast<ArrayValue*>(lhs);
      ArrayValue* rhsArray = dynamic_cast<ArrayValue*>(rhs);
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

  Value* convert(Value* val, TypeSystem::Type* type)
  {
    if(!type->canConvert(val->type))
    {
      INTERNAL_ERROR;
    }
  }
}

