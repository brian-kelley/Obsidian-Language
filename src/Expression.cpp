#include "Expression.hpp"
#include "Variable.hpp"
#include "Scope.hpp"
#include "Subroutine.hpp"

using namespace TypeSystem;

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
  bool rootFinal = isFinal;
  isFinal = false;
  if(!scope)
  {
    ERR_MSG("tried to access member of non-struct");
  }
  //special case: <array>.len
  if(rootFinal && dynamic_cast<ArrayType*>(root->type) && name == "len")
  {
    root = new ArrayLength(root);
    scope = nullptr;
    isFinal = true;
    return;
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
            ERR_MSG("tried to access member variable " <<
                var->name << " in static context");
          }
          //first variable
          root = new VarExpr(var);
        }
        //if var is of a  StructType, iter = struct scope (otherwise null)
        //if there are more names after this and iter is null, will error on
        //  next iteration
        StructType* st = dynamic_cast<StructType*>(root->type);
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
      scope = (Scope*) n.item;
    case Name::STRUCT:
      scope = ((StructType*) n.item)->structScope;
    case Name::TYPEDEF:
    {
      auto at = (AliasType*) n.item;
      if(at->isStruct())
        scope = ((StructType*) at->actual)->structScope;
      else
        scope = nullptr;
    }
    default:
    {
      ERR_MSG("name " << name <<
          " is not a scope, variable or subroutine");
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
  bool typesNull = ltype == NULL || rtype == NULL;
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
      if(typesNull || !(ltype->isInteger()) || !(rtype->isInteger()))
      {
        ERR_MSG("operands to bitwise operators must be integers.");
      }
      //the resulting type is the wider of the two integers, favoring unsigned
      typedef IntegerType IT;
      IT* lhsInt = dynamic_cast<IT*>(ltype);
      IT* rhsInt = dynamic_cast<IT*>(rtype);
      int size = std::max(lhsInt->size, rhsInt->size);
      bool isSigned = lhsInt->isSigned || rhsInt->isSigned;
      //now look up the integer type with given size and signedness
      this->type = getIntegerType(size, isSigned);
      break;
    }
    case PLUS:
    case SUB:
    case MUL:
    case DIV:
    case MOD:
    {
      //TODO (CTE): catch div/mod where rhs = 0
      //TODO: support array concatenation with +
      if(typesNull || !(ltype->isNumber()) || !(rtype->isNumber()))
      {
        ERR_MSG("operands to arithmetic operators must be numbers.");
      }
      this->type = TypeSystem::promote(ltype, rtype);
      break;
    }
    case SHL:
    case SHR:
    {
      //TODO (CTE): if rhs is known, warn if rhs is negative or
      //greater than the width of the lhs type.
      if(typesNull || !(ltype->isInteger()) || !(rtype->isInteger()))
      {
        ERR_MSG("operands to bit shifting operators must be integers.");
      }
      this->type = ltype;
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
      if(typesNull)
      {
        ERR_MSG("can't compare two compound literals.");
      }
      //here, use the canConvert that takes an expression
      if((ltype && ltype->canConvert(r)) || (rtype && rtype->canConvert(l)))
      {
        this->type = primitives[TypeNT::BOOL];
      }
      else
      {
        ERR_MSG("types can't be compared.");
      }
      break;
    }
    default: INTERNAL_ERROR;
  }
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
  //if value fits in a signed int, use that as the type
  //when in doubt, don't use auto
  if(value > 0x7FFFFFFF)
  {
    type = primitives[Parser::TypeNT::ULONG];
  }
  else
  {
    type = primitives[Parser::TypeNT::UINT];
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
  type = TypeSystem::getArrayType(primitives[Parser::TypeNT::CHAR], 1);
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
  //the type of all members, if they are the same
  Type* memberType = memberTypes[0];
  bool isTuple = false;
  for(size_t i = 1; i < members.size(); i++)
  {
    if(memberTypes[i] != memberType)
    {
      memberType = nullptr;
      break;
    }
  }
  if(memberType)
  {
    type = getArrayType(memberType, 1);
  }
  else
  {
    type = getTupleType(memberTypes);
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
    if(!mt->key->canConvert(ind))
    {
      ERR_MSG("used incorrect type to index map");
    }
    type = mt->value;
  }
  else
  {
    ERR_MSG("expression can't be subscripted (is not an array, tuple or map)");
  }
}

/************
 * CallExpr *
 ************/

CallExpr::CallExpr(Expression* c, vector<Expression*>& a)
{
  //callable expressions should only be produced from Expr12, so c actually
  //being a Callable must already have been checked
  if(!dynamic_cast<CallableType*>(c->type))
  {
    ERR_MSG("expression is not callable");
  }
  callable = c;
  args = a;
  checkArgs((CallableType*) callable->type, args);
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
    if(!callable->argTypes[i]->canConvert(args[i]))
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
}

VarExpr::VarExpr(Variable* v)
{
  this->type = v->type;
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
}

/*************
 * StructMem *
 *************/

StructMem::StructMem(Expression* b, Variable* v)
{
  base = b;
  member = v;
  type = v->type;
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
}

/***********
 * TempVar *
 ***********/

TempVar::TempVar(string id, Type* t, Scope* s) : ident(id) {}

