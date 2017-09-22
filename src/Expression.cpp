#include "Expression.hpp"
#include "Variable.hpp"
#include "Scope.hpp"
#include "Subroutine.hpp"

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
  //Get a list of the Expr2s
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
    BinaryArith* chain = new BinaryArith(leaves[0], LOR, leaves[1]);
    for(size_t i = 2; i < leaves.size(); i++)
    {
      //form another BinaryArith with root and expr2[i] as operands
      chain = new BinaryArith(chain, LOR, leaves[i]);
    }
    return chain;
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
  if(expr->e.is<IntLit*>())
  {
    return new IntLiteral(expr->e.get<IntLit*>());
  }
  else if(expr->e.is<FloatLit*>())
  {
    return new FloatLiteral(expr->e.get<FloatLit*>());
  }
  else if(expr->e.is<CharLit*>())
  {
    return new CharLiteral(expr->e.get<CharLit*>());
  }
  else if(expr->e.is<StrLit*>())
  {
    return new StringLiteral(expr->e.get<StrLit*>());
  }
  else if(expr->e.is<Parser::BoolLit*>())
  {
    return new BoolLiteral(expr->e.get<Parser::BoolLit*>());
  }
  else if(expr->e.is<Parser::ExpressionNT*>())
  {
    return getExpression(s, expr->e.get<Parser::ExpressionNT*>());
  }
  else if(expr->e.is<Parser::Member*>())
  {
    return new VarExpr(s, expr->e.get<Parser::Member*>());
  }
  else if(expr->e.is<Parser::StructLit*>())
  {
    return new CompoundLiteral(s, expr->e.get<Parser::StructLit*>());
  }
  else if(expr->e.is<Parser::TupleLit*>())
  {
    return new TupleLiteral(s, expr->e.get<Parser::TupleLit*>());
  }
  else if(expr->e.is<Parser::CallNT*>())
  {
    return new CallExpr(s, expr->e.get<Parser::CallNT*>());
  }
  else if(expr->e.is<Parser::Expr12::ArrayIndex>())
  {
    return new Indexed(s, &(expr->e.get<Parser::Expr12::ArrayIndex>()));
  }
  else if(expr->e.is<Parser::NewArrayNT*>())
  {
    return new NewArray(s, expr->e.get<Parser::NewArrayNT*>());
  }
  else
  {
    cout << "ERROR: Expr12 with tag " << expr->e.which() << '\n';
    INTERNAL_ERROR;
    return NULL;
  }
}

/**************
 * Expression *
 **************/

Expression::Expression(Scope* s)
{
  scope = s;
  //type is set by a subclass constructor
}

/**************
 * UnaryArith *
 **************/

UnaryArith::UnaryArith(int o, Expression* e) : Expression(NULL)
{
  this->op = o;
  this->expr = e;
}

/***************
 * BinaryArith *
 ***************/

BinaryArith::BinaryArith(Expression* l, int o, Expression* r) : Expression(NULL)
{
  using Parser::TypeNT;
  //Type check the operation
  auto ltype = l->type;
  auto rtype = r->type;
  bool typesNull = ltype == NULL || rtype == NULL;
  op = o;
  switch(o)
  {
    case LOR:
    case LAND:
    {
      if(ltype != TypeSystem::primitives[TypeNT::BOOL] ||
         rtype != TypeSystem::primitives[TypeNT::BOOL])
      {
        ERR_MSG("operands to || and && must both be booleans.");
      }
      //type of expression is always bool
      this->type = TypeSystem::primitives[TypeNT::BOOL];
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
      typedef TypeSystem::IntegerType IT;
      IT* lhsInt = dynamic_cast<IT*>(ltype);
      IT* rhsInt = dynamic_cast<IT*>(rtype);
      int size = std::max(lhsInt->size, rhsInt->size);
      bool isSigned = lhsInt->isSigned || rhsInt->isSigned;
      //now look up the integer type with given size and signedness
      this->type = TypeSystem::getIntegerType(size, isSigned);
      break;
    }
    case PLUS:
    case SUB:
    case MUL:
    case DIV:
    case MOD:
    {
      //TODO: warn on div by 0
      if(typesNull || !(ltype->isNumber()) || !(rtype->isNumber()))
      {
        ERR_MSG("operands to arithmetic operators must be numbers.");
      }
      //get type of result as the "most promoted" of ltype and rtype
      //double > float, float > integers, unsigned > signed, wider integer > narrower integer
      if(ltype->isInteger() && rtype->isInteger())
      {
        auto lhsInt = dynamic_cast<TypeSystem::IntegerType*>(ltype);
        auto rhsInt = dynamic_cast<TypeSystem::IntegerType*>(rtype);
        int size = std::max(lhsInt->size, rhsInt->size);
        bool isSigned = lhsInt->isSigned || rhsInt->isSigned;
        //now look up the integer type with given size and signedness
        this->type = TypeSystem::getIntegerType(size, isSigned);
      }
      else if(ltype->isInteger())
      {
        //rtype is floating point, so use that
        this->type = rtype;
      }
      else if(rtype->isInteger())
      {
        this->type = ltype;
      }
      else
      {
        //both floats, so pick the bigger one
        auto lhsFloat = dynamic_cast<TypeSystem::FloatType*>(ltype);
        auto rhsFloat = dynamic_cast<TypeSystem::FloatType*>(rtype);
        if(lhsFloat->size >= rhsFloat->size)
        {
          this->type = ltype;
        }
        else
        {
          this->type = rtype;
        }
      }
      break;
    }
    case SHL:
    case SHR:
    {
      //TODO: if rhs is a constant, warn if evaluates to negative or greater than the width of the lhs type.
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
      //Can't directly compare two compound literals (ok because there is no reason to do that)
      //To determine if comparison is allowed, lhs or rhs needs to be convertible to the type of the other
      if(typesNull)
      {
        ERR_MSG("can't compare two compound literals.");
      }
      //here, use the canConvert that takes an expression
      if((ltype && ltype->canConvert(r)) || (rtype && rtype->canConvert(l)))
      {
        this->type = TypeSystem::primitives[TypeNT::BOOL];
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

IntLiteral::IntLiteral(IntLit* ast) : Expression(NULL), value(ast->val)
{
  setType();
}

IntLiteral::IntLiteral(uint64_t val) : Expression(NULL), value(val)
{
  setType();
}

void IntLiteral::setType()
{
  //if value fits in a signed int, use that as the type
  //when in doubt, don't use auto
  if(value > 0x7FFFFFFF)
  {
    type = TypeSystem::primitives[Parser::TypeNT::ULONG];
  }
  else
  {
    type = TypeSystem::primitives[Parser::TypeNT::UINT];
  }
}

FloatLiteral::FloatLiteral(FloatLit* a) : Expression(NULL), value(a->val)
{
  type = TypeSystem::primitives[Parser::TypeNT::DOUBLE];
}

FloatLiteral::FloatLiteral(double val) : Expression(NULL), value(val)
{
  type = TypeSystem::primitives[Parser::TypeNT::DOUBLE];
}

StringLiteral::StringLiteral(StrLit* a) : Expression(NULL)
{
  value = a->val;
  type = TypeSystem::primitives[Parser::TypeNT::CHAR]->getArrayType(1);
}

CharLiteral::CharLiteral(CharLit* a) : Expression(NULL)
{
  value = a->val;
  type = TypeSystem::primitives[Parser::TypeNT::CHAR];
}

BoolLiteral::BoolLiteral(Parser::BoolLit* a) : Expression(NULL)
{
  value = a->val;
  type = TypeSystem::primitives[Parser::TypeNT::BOOL];
}

/*******************
 * CompoundLiteral *
 *******************/

CompoundLiteral::CompoundLiteral(Scope* s, Parser::StructLit* a) : Expression(s)
{
  this->ast = a;
  //type cannot be determined for a compound literal
  type = NULL;
  for(auto v : ast->vals)
  {
    //add member expression
    members.push_back(getExpression(s, v));
  }
}

/****************
 * TupleLiteral *
 ****************/

TupleLiteral::TupleLiteral(Scope* s, Parser::TupleLit* a) : Expression(s)
{
  this->ast = a;
  vector<TypeSystem::Type*> memTypes;
  bool typeResolved = true;
  for(auto it : ast->vals)
  {
    members.push_back(getExpression(s, it));
    memTypes.push_back(members.back()->type);
    if(!memTypes.back())
    {
      typeResolved = false;
    }
  }
  //if all members' types are known, can get this type also (otherwise leave it null)
  if(typeResolved)
  {
    type = new TypeSystem::TupleType(memTypes);
  }
}

/***********
 * Indexed *
 ***********/

Indexed::Indexed(Scope* s, Parser::Expr12::ArrayIndex* a) : Expression(s)
{
  this->ast = a;
  //get expressions for the index and the indexed object
  group = getExpression(s, ast->arr);
  index = getExpression(s, ast->index);
  //Indexing a CompoundLiteral is not allowed at all
  //Indexing a Tuple (literal, variable or call) requires the index to be an IntLit
  //Anything else is assumed to be an array and then the index can be any integer expression
  if(dynamic_cast<CompoundLiteral*>(group))
  {
    ERR_MSG("Can't index a compound literal - assign it to an array first.");
  }
  //note: ok if this is null
  //in all other cases, group must have a type now
  if(auto tt = dynamic_cast<TypeSystem::TupleType*>(group->type))
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
  else if(auto at = dynamic_cast<TypeSystem::ArrayType*>(group->type))
  {
    //group must be an array
    type = at->subtype;
  }
  else
  {
    ERR_MSG("expression can't be subscripted.");
  }
}

/************
 * CallExpr *
 ************/

CallExpr::CallExpr(Scope* s, Parser::CallNT* ast) : Expression(s)
{
  subr = s->findSubroutine(ast->callable);
  if(!subr)
  {
    ERR_MSG("\"" << ast->callable << "\" is not a function or procedure");
  }
  args.resize(ast->args.size());
  for(size_t i = 0; i < args.size(); i++)
  {
    args[i] = getExpression(s, ast->args[i]);
  }
  this->type = subr->retType;
}

/***********
 * VarExpr *
 ***********/

VarExpr::VarExpr(Scope* s, Parser::Member* ast) : Expression(s)
{
  //To get type and var (Variable*), look up the variable in scope tree
  var = s->findVariable(ast);
  if(!var)
  {
    ERR_MSG("Use of undeclared variable " << *ast);
  }
  //type of variable must be known
  this->type = var->type;
}

VarExpr::VarExpr(Scope* s, Variable* v) : Expression(s)
{
  this->type = v->type;
}

/************
 * NewArray *
 ************/

NewArray::NewArray(Scope* s, Parser::NewArrayNT* ast) : Expression(s)
{
  auto elemType = TypeSystem::lookupType(ast->elemType, s);
  this->type = elemType->getArrayType(ast->dimensions.size());
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

