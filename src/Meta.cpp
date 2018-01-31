#include "Meta.hpp"

#include "TypeSystem.hpp"
#include "Expression.hpp"
#include "Subroutine.hpp"

using namespace TypeSystem;

map<Variable*, Value*> varValues;

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
    Value* callable = expression(c->callable);
    vector<Value*> args;
    for(auto a : c->args)
    {
      args.push_back(expression(a));
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
      //maps identical, lhs < rhs false
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
  }
}

