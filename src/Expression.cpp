#include "Expression.hpp"
#include "Variable.hpp"
#include "Scope.hpp"
#include "Subroutine.hpp"

using namespace TypeSystem;

bool Expression::pureWithin(Scope* s)
{
  return pure && withinScope(s);
}

bool Expression::withinScope(Scope* s)
{
  for(auto dep : deps)
  {
    //from var's scope, walk up, looking for s
    //if global is reached first, return false
    bool depInS = false;
    for(Scope* iter = dep->scope; iter; iter = iter->parent)
    {
      if(iter == s)
      {
        //dep does live in s, so it's OK
        depInS = true;
        break;
      }
    }
    if(!depInS)
      return false;
  }
  return true;
}

/**********************
 * Expression loading *
 **********************/

template<> Expression* getExpression<Parser::Expr1>(Scope* s, Parser::Expr1* ast);
template<> Expression* getExpression<Parser::Expr2>(Scope* s, Parser::Expr2* ast);
template<> Expression* getExpression<Parser::Expr3>(Scope* s, Parser::Expr3* ast);
template<> Expression* getExpression<Parser::Expr4>(Scope* s, Parser::Expr4* ast);
template<> Expression* getExpression<Parser::Expr5>(Scope* s, Parser::Expr5* ast);
template<> Expression* getExpression<Parser::Expr6>(Scope* s, Parser::Expr6* ast);
template<> Expression* getExpression<Parser::Expr7>(Scope* s, Parser::Expr7* ast);
template<> Expression* getExpression<Parser::Expr8>(Scope* s, Parser::Expr8* ast);
template<> Expression* getExpression<Parser::Expr9>(Scope* s, Parser::Expr9* ast);
template<> Expression* getExpression<Parser::Expr10>(Scope* s, Parser::Expr10* ast);
template<> Expression* getExpression<Parser::Expr11>(Scope* s, Parser::Expr11* ast);
template<> Expression* getExpression<Parser::Expr12>(Scope* s, Parser::Expr12* ast);

template<>
Expression* getExpression<Parser::Expr1>(Scope* s, Parser::Expr1* expr)
{
  //Expr1 can either be an "array Type[dim1][dim2]...[dimN]"
  //expression or a binary expr chain like the others
  if(expr->e.is<Parser::NewArrayNT*>())
  {
    return new NewArray(s, expr->e.get<Parser::NewArrayNT*>());
  }
  else
  {
    //Get a list of the Expr2s
    vector<Expression*> leaves;
    leaves.push_back(getExpression(s, expr->e.get<Parser::Expr2*>()));
    for(auto e : expr->tail)
    {
      leaves.push_back(getExpression(s, e->rhs));
    }
    if(leaves.size() == 1)
    {
      return leaves.front();
    }
    else
    {
      //build chain of BinaryAriths that evaluates left to right
      BinaryArith* chain = new BinaryArith(leaves[0], LOR, leaves[1]);
      for(size_t i = 2; i < leaves.size(); i++)
      {
        //form another BinaryArith with root and expr2[i] as operands
        chain = new BinaryArith(chain, LOR, leaves[i]);
      }
      return chain;
    }
  }
}

template<>
Expression* getExpression<Parser::Expr2>(Scope* s, Parser::Expr2* expr)
{
  vector<Expression*> leaves;
  leaves.push_back(getExpression(s, expr->head));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs));
  }
  if(leaves.size() == 1)
  {
    return leaves.front();
  }
  else
  {
    //build chain of BinaryAriths that evaluates left to right
    BinaryArith* chain = new BinaryArith(leaves[0], LAND, leaves[1]);
    for(size_t i = 2; i < leaves.size(); i++)
    {
      //form another BinaryArith with root and expr2[i] as operands
      chain = new BinaryArith(chain, LAND, leaves[i]);
    }
    return chain;
  }
}

template<>
Expression* getExpression<Parser::Expr3>(Scope* s, Parser::Expr3* expr)
{
  vector<Expression*> leaves;
  leaves.push_back(getExpression(s, expr->head));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs));
  }
  if(leaves.size() == 1)
  {
    return leaves.front();
  }
  else
  {
    //build chain of BinaryAriths that evaluates left to right
    BinaryArith* chain = new BinaryArith(leaves[0], BOR, leaves[1]);
    //all expressions in a chain of logical AND must be bools
    for(auto e : leaves)
    {
      if(e->type == NULL || !e->type->isInteger())
      {
        ERR_MSG("operands to && must both be booleans.");
      }
    }
    for(size_t i = 2; i < leaves.size(); i++)
    {
      //form another BinaryArith with root and expr2[i] as operands
      chain = new BinaryArith(chain, BOR, leaves[i]);
    }
    return chain;
  }
}

template<>
Expression* getExpression<Parser::Expr4>(Scope* s, Parser::Expr4* expr)
{
  vector<Expression*> leaves;
  leaves.push_back(getExpression(s, expr->head));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs));
  }
  if(leaves.size() == 1)
  {
    return leaves.front();
  }
  else
  {
    //build chain of BinaryAriths that evaluates left to right
    BinaryArith* chain = new BinaryArith(leaves[0], BXOR, leaves[1]);
    for(size_t i = 2; i < leaves.size(); i++)
    {
      //form another BinaryArith with root and expr2[i] as operands
      chain = new BinaryArith(chain, BXOR, leaves[i]);
    }
    return chain;
  }
}

template<>
Expression* getExpression<Parser::Expr5>(Scope* s, Parser::Expr5* expr)
{
  vector<Expression*> leaves;
  leaves.push_back(getExpression(s, expr->head));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs));
  }
  if(leaves.size() == 1)
  {
    return leaves.front();
  }
  else
  {
    //build chain of BinaryAriths that evaluates left to right
    BinaryArith* chain = new BinaryArith(leaves[0], BAND, leaves[1]);
    for(size_t i = 2; i < leaves.size(); i++)
    {
      //form another BinaryArith with root and expr2[i] as operands
      chain = new BinaryArith(chain, BAND, leaves[i]);
    }
    return chain;
  }
}

template<>
Expression* getExpression<Parser::Expr6>(Scope* s, Parser::Expr6* expr)
{
  vector<Expression*> leaves;
  leaves.push_back(getExpression(s, expr->head));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs));
  }
  if(leaves.size() == 1)
  {
    return leaves.front();
  }
  else
  {
    //build chain of BinaryAriths that evaluates left to right
    BinaryArith* chain = new BinaryArith(leaves[0], expr->tail[0]->op, leaves[1]);
    for(size_t i = 2; i < leaves.size(); i++)
    {
      //form another BinaryArith with root and expr2[i] as operands
      chain = new BinaryArith(chain, expr->tail[i - 1]->op, leaves[i]);
    }
    return chain;
  }
}

template<>
Expression* getExpression<Parser::Expr7>(Scope* s, Parser::Expr7* expr)
{
  vector<Expression*> leaves;
  leaves.push_back(getExpression(s, expr->head));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs));
  }
  if(leaves.size() == 1)
  {
    return leaves.front();
  }
  else
  {
    //build chain of BinaryAriths that evaluates left to right
    BinaryArith* chain = new BinaryArith(leaves[0], expr->tail[0]->op, leaves[1]);
    for(size_t i = 2; i < leaves.size(); i++)
    {
      //form another BinaryArith with root and expr2[i] as operands
      chain = new BinaryArith(chain, expr->tail[i - 1]->op, leaves[i]);
    }
    return chain;
  }
}

template<>
Expression* getExpression<Parser::Expr8>(Scope* s, Parser::Expr8* expr)
{
  vector<Expression*> leaves;
  leaves.push_back(getExpression(s, expr->head));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs));
  }
  if(leaves.size() == 1)
  {
    return leaves.front();
  }
  else
  {
    //build chain of BinaryAriths that evaluates left to right
    BinaryArith* chain = new BinaryArith(leaves[0], expr->tail[0]->op, leaves[1]);
    for(size_t i = 2; i < leaves.size(); i++)
    {
      //form another BinaryArith with root and expr2[i] as operands
      chain = new BinaryArith(chain, expr->tail[i - 1]->op, leaves[i]);
    }
    return chain;
  }
}

template<>
Expression* getExpression<Parser::Expr9>(Scope* s, Parser::Expr9* expr)
{
  vector<Expression*> leaves;
  leaves.push_back(getExpression(s, expr->head));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs));
  }
  if(leaves.size() == 1)
  {
    return leaves.front();
  }
  else
  {
    //build chain of BinaryAriths that evaluates left to right
    BinaryArith* chain = new BinaryArith(leaves[0], expr->tail[0]->op, leaves[1]);
    for(size_t i = 2; i < leaves.size(); i++)
    {
      //form another BinaryArith with root and expr2[i] as operands
      chain = new BinaryArith(chain, expr->tail[i - 1]->op, leaves[i]);
    }
    return chain;
  }
}

template<>
Expression* getExpression<Parser::Expr10>(Scope* s, Parser::Expr10* expr)
{
  vector<Expression*> leaves;
  leaves.push_back(getExpression(s, expr->head));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs));
  }
  if(leaves.size() == 1)
  {
    return leaves.front();
  }
  else
  {
    //build chain of BinaryAriths that evaluates left to right
    BinaryArith* chain = new BinaryArith(leaves[0], expr->tail[0]->op, leaves[1]);
    for(size_t i = 2; i < leaves.size(); i++)
    {
      //form another BinaryArith with root and expr2[i] as operands
      chain = new BinaryArith(chain, expr->tail[i - 1]->op, leaves[i]);
    }
    return chain;
  }
}

template<>
Expression* getExpression<Parser::Expr11>(Scope* s, Parser::Expr11* expr)
{
  if(expr->e.is<Parser::Expr12*>())
  {
    return getExpression(s, expr->e.get<Parser::Expr12*>());
  }
  else
  {
    //unary expression, with a single Expr11 as the operand
    auto unary = expr->e.get<Parser::Expr11::UnaryExpr>();
    Expression* operand = getExpression(s, unary.rhs);
    return new UnaryArith(unary.op, operand);
  }
}

template<>
Expression* getExpression<Parser::Expr12>(Scope* s, Parser::Expr12* expr)
{
  Expression* root = nullptr;
  if(expr->e.is<IntLit*>())
  {
    root = new IntLiteral(expr->e.get<IntLit*>());
  }
  else if(expr->e.is<FloatLit*>())
  {
    root = new FloatLiteral(expr->e.get<FloatLit*>());
  }
  else if(expr->e.is<CharLit*>())
  {
    root = new CharLiteral(expr->e.get<CharLit*>());
  }
  else if(expr->e.is<StrLit*>())
  {
    root = new StringLiteral(expr->e.get<StrLit*>());
  }
  else if(expr->e.is<Parser::BoolLit*>())
  {
    root = new BoolLiteral(expr->e.get<Parser::BoolLit*>());
  }
  else if(expr->e.is<Parser::ExpressionNT*>())
  {
    root = getExpression(s, expr->e.get<Parser::ExpressionNT*>());
  }
  else if(expr->e.is<Parser::Member*>())
  {
    //leave root null, and start searching names 
    auto mem = expr->e.get<Parser::Member*>();
    Scope* search = s;
    bool done = false;
    processExpr12Name(mem->names[0], done, true, root, search);
    for(size_t i = 1; i < mem->names.size(); i++)
    {
      processExpr12Name(mem->names[i], done, false, root, search);
    }
    if(!done)
    {
      ERR_MSG("name " << mem->names.back() << " is not a valid expression");
    }
  }
  else if(expr->e.is<Parser::StructLit*>())
  {
    root = new CompoundLiteral(s, expr->e.get<Parser::StructLit*>());
  }
  else if(expr->e.is<Parser::Expr12::This>())
  {
    root = new ThisExpr(s);
  }
  else if(expr->e.is<Parser::Expr12::Error>())
  {
    root = new ErrorVal;
  }
  else
  {
    //some option for the Expr12::e variant wasn't covered here
    //(a simple error in the compiler)
    INTERNAL_ERROR;
  }
  Scope* search = nullptr;
  if(auto st = dynamic_cast<StructType*>(root->type))
  {
    search = st->structScope;
  }
  bool isFinal = true;
  for(size_t i = 0; i < expr->tail.size(); i++)
  {
    //apply each rhs to e12 to get the final expression
    //consective names are handled in a group by applyNamesToExpr12,
    //so store the names in rhsNames as they are encountered
    //Call and index operators are handled here, one at a time
    auto& e12rhs = expr->tail[i]->e;
    if(e12rhs.is<string>())
    {
      //special case: len as a member of array
      string name = e12rhs.get<string>();
      processExpr12Name(name, isFinal, false, root, search);
    }
    else if(e12rhs.is<Parser::CallOp*>())
    {
      vector<Expression*> args;
      auto co = e12rhs.get<Parser::CallOp*>();
      for(auto arg : co->args)
      {
        args.push_back(getExpression(s, arg));
      }
      root = new CallExpr(root, args);
    }
    else if(e12rhs.is<Parser::ExpressionNT*>())
    {
      //array indexing
      Expression* index = getExpression(s, e12rhs.get<Parser::ExpressionNT*>());
      root = new Indexed(root, index);
    }
    else
    {
      //(probably) something wrong with parser
      INTERNAL_ERROR;
    }
  }
  if(!isFinal)
  {
    ERR_MSG("invalid expression");
  }
  return root;
}

void processExpr12Name(string name, bool& isFinal, bool first, Expression*& root, Scope*& scope)
{
  string path = scope ? scope->getFullPath() : "<null scope>";
  bool rootFinal = isFinal;
  isFinal = false;
  //special case: <array>.len
  if(rootFinal && dynamic_cast<ArrayType*>(root->type) && name == "len")
  {
    root = new ArrayLength(root);
    scope = nullptr;
    isFinal = true;
    return;
  }
  StructType* st = nullptr;
  //before doing scope lookup, if root is a struct or bounded type,
  //try to look up a subroutine
  if(root && rootFinal)
  {
    st = dynamic_cast<StructType*>(root->type);
    if(st)
    {
      auto ifaceIt = st->interface.find(name);
      if(ifaceIt != st->interface.end())
      {
        //is a subroutine member of iface
        if(ifaceIt->second.member)
        {
          root = new SubroutineExpr(new StructMem(
                root, ifaceIt->second.member), ifaceIt->second.subr);
        }
        else
        {
          root = new SubroutineExpr(root, ifaceIt->second.subr);
        }
        isFinal = true;
        scope = nullptr;
        return;
      }
    }
  }
  if(!scope)
  {
    ERR_MSG("tried to access member of non-struct");
  }
  Name n;
  if(first)
    n = scope->findName(name);
  else
    n = scope->lookup(name);
  if(n.item == nullptr)
  {
    //name doesn't exist at all
    ERR_MSG("use of undeclared identifier " << name);
  }
  switch(n.kind)
  {
    case Name::VARIABLE:
      {
        Variable* var = (Variable*) n.item;
        if(root)
        {
          if(!var->isMember)
          {
            ERR_MSG("tried to access static variable " <<
                var->name << " as member");
          }
          root = new StructMem(root, var);
        }
        else
        {
          if(var->isMember)
          {
            //have no root but referencing non-static member
            //use a StructMem(ThisExpr, var)
            //if in static context, ThisExpr ctor will print error
            root = new StructMem(new ThisExpr(scope), var);
          }
          else
          {
            //just a standalone (static) variable
            root = new VarExpr(var);
          }
        }
        //update the search scope using new root expression type
        st = dynamic_cast<StructType*>(root->type);
        if(st)
          scope = st->structScope;
        else
          scope = nullptr;
        isFinal = true;
        break;
      }
    case Name::SUBROUTINE:
      {
        //callable (could be method or standalone)
        if(root)
          root = new SubroutineExpr(root, (Subroutine*) n.item);
        else
          root = new SubroutineExpr((Subroutine*) n.item);
        scope = nullptr;
        isFinal = true;
        break;
      }
    case Name::MODULE:
      {
        scope = (Scope*) n.item;
        break;
      }
    case Name::STRUCT:
      {
        scope = ((StructType*) n.item)->structScope;
        break;
      }
    case Name::TYPEDEF:
      {
        auto at = (AliasType*) n.item;
        if(at->isStruct())
          scope = ((StructType*) at->actual)->structScope;
        else
          scope = nullptr;
        break;
      }
    case Name::ENUM_CONSTANT:
      {
        EnumConstant* ec = (EnumConstant*) n.item;
        if(!root)
        {
          root = new EnumExpr(ec);
          isFinal = true;
          scope = nullptr;
        }
        else
        {
          ERR_MSG("enum constant " << ec->name << " can't be used as member");
        }
        break;
      }
    default:
      {
        ERR_MSG("name " << name <<
            " is not a scope, variable, subroutine or enum constant");
      }
  }
}

StructScope* scopeForExpr(Expression* expr)
{
  Type* t = expr->type;
  if(!t)
  {
    ERR_MSG("cannot directly access members of compound literal");
  }
  StructType* st = dynamic_cast<StructType*>(t);
  if(!st)
  {
    ERR_MSG("cannot access members of non-struct type");
  }
  return st->structScope;
}

/**************
 * UnaryArith *
 **************/

UnaryArith::UnaryArith(int o, Expression* e)
{
  this->op = o;
  this->expr = e;
  deps = e->deps;
  pure = e->pure;
  type = e->type;
}

/***************
 * BinaryArith *
 ***************/

BinaryArith::BinaryArith(Expression* l, int o, Expression* r) : lhs(l), rhs(r)
{
  using Parser::TypeNT;
  //Type check the operation
  auto ltype = lhs->type;
  auto rtype = rhs->type;
  op = o;
  switch(o)
  {
    case LOR:
    case LAND:
    {
      if(ltype != primitives[TypeNT::BOOL] ||
         rtype != primitives[TypeNT::BOOL])
      {
        ERR_MSG("operands to || and && must both be booleans.");
      }
      //type of expression is always bool
      this->type = primitives[TypeNT::BOOL];
      break;
    }
    case BOR:
    case BAND:
    case BXOR:
    {
      //both operands must be integers
      if(!(ltype->isInteger()) || !(rtype->isInteger()))
      {
        ERR_MSG("operands to bitwise operators must be integers.");
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
          ERR_MSG("incompatible array concatenation operands: " <<
              ltype->getName() << " and " << rtype->getName());
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
          ERR_MSG("can't append type " << rtype->getName() <<
              " to " << ltype->getName());
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
          ERR_MSG("can't prepend type " << ltype->getName() <<
              " to " << rtype->getName());
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
        ERR_MSG("operands to arithmetic operators must be numbers.");
      }
      type = TypeSystem::promote(ltype, rtype);
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
        ERR_MSG("operands to bit shifting operators must be integers.");
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
      type = primitives[TypeNT::BOOL];
      if(!ltype->canConvert(rtype) && !rtype->canConvert(ltype))
      {
        ERR_MSG("can't compare " << ltype->getName() <<
            " and " << rtype->getName());
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
  deps.insert(l->deps.begin(), l->deps.end());
  deps.insert(r->deps.begin(), r->deps.end());
  pure = l->pure && r->pure;
}

/**********************
 * Primitive Literals *
 **********************/

IntLiteral::IntLiteral(IntLit* ast) : value(ast->val)
{
  setType();
}

IntLiteral::IntLiteral(uint64_t val) : value(val)
{
  setType();
}

void IntLiteral::setType()
{
  //use int32 (or int64 if too big for 32)
  //this constant is INT_MAX
  if(value > 0x7FFFFFFF)
  {
    type = primitives[Parser::TypeNT::LONG];
  }
  else
  {
    type = primitives[Parser::TypeNT::INT];
  }
}

FloatLiteral::FloatLiteral(FloatLit* a) : value(a->val)
{
  type = primitives[Parser::TypeNT::DOUBLE];
}

FloatLiteral::FloatLiteral(double val) : value(val)
{
  type = primitives[Parser::TypeNT::DOUBLE];
}

StringLiteral::StringLiteral(StrLit* a)
{
  value = a->val;
  type = getArrayType(primitives[Parser::TypeNT::CHAR], 1);
}

CharLiteral::CharLiteral(CharLit* a)
{
  value = a->val;
  type = primitives[Parser::TypeNT::CHAR];
}

BoolLiteral::BoolLiteral(Parser::BoolLit* a)
{
  value = a->val;
  type = primitives[Parser::TypeNT::BOOL];
}

/*******************
 * CompoundLiteral *
 *******************/

CompoundLiteral::CompoundLiteral(Scope* s, Parser::StructLit* a)
{
  this->ast = a;
  //this is an lvalue if all of its members are lvalues
  lvalue = true;
  for(auto v : ast->vals)
  {
    //add member expression
    members.push_back(getExpression(s, v));
    if(!members.back()->assignable())
    {
      lvalue = false;
    }
  }
  //get type (must preserve all information about member types)
  //so, if all members have the same type, is a 1-dim array of that
  //otherwise is a tuple
  vector<Type*> memberTypes;
  for(auto mem : members)
  {
    memberTypes.push_back(mem->type);
  }
  type = getTupleType(memberTypes);
  for(auto mem : members)
  {
    deps.insert(mem->deps.begin(), mem->deps.end());
    pure = pure && mem->pure;
  }
}

/***********
 * Indexed *
 ***********/

Indexed::Indexed(Expression* grp, Expression* ind)
{
  group = grp;
  index = ind;
  //Indexing a CompoundLiteral is not allowed at all
  //Indexing a Tuple (literal, variable or call) requires the index to be an IntLit
  //Anything else is assumed to be an array and then the index can be any integer expression
  if(dynamic_cast<CompoundLiteral*>(group))
  {
    ERR_MSG("Can't index a compound literal - assign it to an array first.");
  }
  //note: ok if this is null
  //in all other cases, group must have a type now
  if(auto tt = dynamic_cast<TupleType*>(group->type))
  {
    //group's type is a Tuple, whether group is a literal, var or call
    //make sure the index is an IntLit
    auto intIndex = dynamic_cast<IntLiteral*>(index);
    if(intIndex)
    {
      //int literals are always unsigned (in lexer) so always positive
      auto val = intIndex->value;
      if(val >= tt->members.size())
      {
        ERR_MSG(string("Tuple subscript out of bounds: tuple has ") + to_string(tt->members.size()) + " but requested member " + to_string(val));
      }
      type = tt->members[val];
    }
    else
    {
      ERR_MSG("Tuple subscript must be an integer constant.");
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
    if(!mt->key->canConvert(ind->type))
    {
      ERR_MSG("used incorrect type to index map");
    }
    //map lookup can fail, so return a "maybe" of value
    type = maybe(mt->value);
  }
  else
  {
    ERR_MSG("expression can't be subscripted (is not an array, tuple or map)");
  }
  deps.insert(grp->deps.begin(), grp->deps.end());
  deps.insert(ind->deps.begin(), ind->deps.end());
  pure = grp->pure && ind->pure;
}

/************
 * CallExpr *
 ************/

CallExpr::CallExpr(Expression* c, vector<Expression*>& a)
{
  //callable expressions should only be produced from Expr12, so c actually
  //being a Callable must already have been checked
  auto ct = dynamic_cast<CallableType*>(c->type);
  if(!ct)
  {
    ERR_MSG("expression is not callable");
  }
  callable = c;
  args = a;
  checkArgs(ct, args);
  deps.insert(c->deps.begin(), c->deps.end());
  pure = pure && c->pure;
  for(auto arg : args)
  {
    deps.insert(arg->deps.begin(), arg->deps.end());
    pure = pure && arg->pure;
  }
  pure = pure && ct->pure;
  type = ct->returnType;
}

void checkArgs(CallableType* callable, vector<Expression*>& args)
{
  //make sure number of arguments matches
  if(callable->argTypes.size() != args.size())
  {
    ERR_MSG("in call to " << (callable->ownerStruct ? "" : "static") <<
        (callable->pure ? "function" : "procedure") << ", expected " <<
        callable->argTypes.size() << " arguments but got " << args.size());
  }
  for(size_t i = 0; i < args.size(); i++)
  {
    //make sure arg value can be converted to expected type
    if(!callable->argTypes[i]->canConvert(args[i]->type))
    {
      ERR_MSG("argument " << i + 1 << " to " << (callable->ownerStruct ? "" : "static") <<
        (callable->pure ? "function" : "procedure") << " has wrong type (expected " <<
        callable->argTypes[i]->getName() << " but got " <<
        (args[i]->type ? args[i]->type->getName() : "incompatible compound literal") << ")");
    }
  }
}

/***********
 * VarExpr *
 ***********/

VarExpr::VarExpr(Scope* s, Parser::Member* ast)
{
  //To get type and var (Variable*), look up the variable in scope tree
  Name n = s->findName(ast);
  if(n.item == nullptr)
  {
    ERR_MSG("use of undeclared identifier " << *ast);
  }
  else if(n.kind != Name::VARIABLE)
  {
    ERR_MSG(*ast << " is not a variable");
  }
  var = (Variable*) n.item;
  //type of variable must be known
  this->type = var->type;
  deps.insert(var);
}

VarExpr::VarExpr(Variable* v) : var(v)
{
  this->type = var->type;
  deps.insert(var);
}

/******************
 * SubroutineExpr *
 ******************/

SubroutineExpr::SubroutineExpr(Subroutine* s)
{
  thisObject = nullptr;
  this->subr = s;
  type = s->type;
}

SubroutineExpr::SubroutineExpr(Expression* root, Subroutine* s)
{
  thisObject = root;
  this->subr = s;
  type = s->type;
  deps.insert(root->deps.begin(), root->deps.end());
  pure = root->pure;
}

/*************
 * StructMem *
 *************/

StructMem::StructMem(Expression* b, Variable* v)
{
  base = b;
  member = v;
  type = v->type;
  deps.insert(b->deps.begin(), b->deps.end());
  pure = b->pure;
}

/************
 * NewArray *
 ************/

NewArray::NewArray(Scope* s, Parser::NewArrayNT* ast)
{
  auto elemType = lookupType(ast->elemType, s);
  this->type = TypeSystem::getArrayType(elemType, ast->dimensions.size());
  for(auto dim : ast->dimensions)
  {
    dims.push_back(getExpression(s, dim));
  }
  //make sure all dimensions are integers
  for(auto dim : dims)
  {
    if(!dim->type->isInteger())
    {
      ERR_MSG("array dimensions must be integers");
    }
  }
}

/***************
 * ArrayLength *
 ***************/

ArrayLength::ArrayLength(Expression* arr)
{
  array = arr;
  this->type = primitives[Parser::TypeNT::UINT];
  deps.insert(arr->deps.begin(), arr->deps.end());
}

/************
 * ThisExpr *
 ************/

ThisExpr::ThisExpr(Scope* where)
{
  //figure out which struct "this" refers to,
  //or show error if there is none
  for(Scope* iter = where; iter; iter = iter->parent)
  {
    auto subrScope = dynamic_cast<SubroutineScope*>(iter);
    auto structScope = dynamic_cast<StructScope*>(iter);
    if(subrScope)
    {
      structType = subrScope->subr->type->ownerStruct;
      break;
    }
    else if(structScope)
    {
      structType = structScope->type;
      break;
    }
  }
  if(!structType)
  {
    ERR_MSG("this pointer not available in static context");
  }
  type = structType;
}

/*************
 * Converted *
 *************/

Converted::Converted(Expression* val, Type* dst)
{
  value = val;
  type = dst;
  if(!type->canConvert(value->type))
  {
    ERR_MSG("can't implicitly convert from " << val->type->getName() << " to " << type->getName());
  }
}

/************
 * EnumExpr *
 ************/

EnumExpr::EnumExpr(TypeSystem::EnumConstant* ec)
{
  type = ec->et;
  value = ec->value;
}

/*********
 * Error *
 *********/

ErrorVal::ErrorVal()
{
  type = primitives[Parser::TypeNT::ERROR];
}

/***********
 * TempVar *
 ***********/

TempVar::TempVar(string id, Type* t, Scope* s) : ident(id) {}

