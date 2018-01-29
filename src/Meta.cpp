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
        if(!operand->type->isInteger())
        {
          ERR_MSG("Invalid operand to bitwise ~ operator");
        }
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
      }
      else if(ua->op == LNOT)
      {
        if(operand->type != primitives[Prim::BOOL])
        {
          ERR_MSG("Invalid operand to boolean ! operator");
        }
        PrimitiveValue* pv = new PrimitiveValue(*((PrimitiveValue*) operand));
        pv->data.b = !pv->data.b;
        return pv;
      }
      else if(ua->op == SUB)
      {
        if(!operand->type->isNumber())
        {
          ERR_MSG("Invalid operand to unary -");
        }
        //can't directly negate unsigned value:
        //there should be a conversion applied first in that case
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
      }
      else
      {
        ERR_MSG("meta: invalid unary operator");
      }
    }
    else if(BinaryArith* ba = dynamic_cast<BinaryArith*>(expr))
    {
      Value* lhs = expression(ba->lhs);
      Value* rhs = expression(ba->rhs);
      switch(ba->op)
      {
        case PLUS:
          //plus is special: can be concat, prepend or append
        case SUB:
        case MUL:
        case DIV:
        case MOD:
        case LOR:
        case BOR:
        case BXOR:
        case LNOT:
        case BNOT:
        case LAND:
        case BAND:
        case SHL:
        case SHR:
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
}

