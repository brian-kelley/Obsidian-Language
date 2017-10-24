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
    auto member = expr->e.get<Parser::Member*>();
    //lookup name
    vector<string> ident;
    for(auto id : member->head)
    {
      ident.push_back(id->name);
    }
    ident.push_back(member->tail->name);
    vector<string> remain;
    Name name;
    if(!(s->lookup(ident, name, remain)))
    {
      ERR_MSG("no variable named " << *member);
    }
    //name can be a variable or subroutine
    if(name.type == Name::VARIABLE)
    {
      root = new VarExpr(s, (Variable*) name.item);
    }
    else if(name.type == Name::SUBROUTINE)
    {
      root = new SubroutineExpr(s, (Subroutine*) name.item);
    }
    else
    {
      ERR_MSG("name does not refer to an expression");
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
  if(e12->tail.size())
  {
    //apply each rhs to e12 to get the final expression
  }
  return root;
}

Expression* applyExpr12RHS(Scope* s, Expression* root, Expr12RHS* e12)
{
  if(e12->e.is<Ident*>())
  {
    //special case: <array>.len
    //len is not a keyword so it's allowed for a non-array to have "len" as member
    auto id = e12->e.get<Ident*>();
    if(root->type && root->type->isArray() && id->name == "len")
    {
      return new ArrayLength(s, root);
    }
    return new StructMem(s, root, e12->e.get<Ident*>()->name);
  }
  else if(e12->e.is<CallOp*>())
  {
    //method call (root is the object)
    //first, make sure that root is a struct
    auto st = dynamic_cast<StructType*>(root->type);
    if(!st)
    {
      ERR_MSG("tried to call method on expression of a non-struct type");
    }
    auto co = e12->e.get<CallOp*>();
    return new MethodExpr(s, root, Subroutine* subr, vector<Expression*>& args);
  }
  else if(e12->e.is<ExpressionNT*>())
  {
    return new ArrayIndex(s, root, getExpression(s, e12->e.get<ExpressionNT*>()));
  }
}

/**************
 * Expression *
 **************/

Expression::Expression(Scope* s)
{
  scope = s;
  //expression type is set by subclass constructors
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

BinaryArith::BinaryArith(Expression* l, int o, Expression* r) : Expression(NULL), lhs(l), rhs(r)
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
      //TODO: warn on div by 0 (when RHS is known at compile-time)
      if(typesNull || !(ltype->isNumber()) || !(rtype->isNumber()))
      {
        ERR_MSG("operands to arithmetic operators must be numbers.");
      }
      //get type of result as the "most promoted" of ltype and rtype
      //double > float, float > integers, unsigned > signed, wider integer > narrower integer
      if(ltype->isInteger() && rtype->isInteger())
      {
        auto lhsInt = dynamic_cast<IntegerType*>(ltype);
        auto rhsInt = dynamic_cast<IntegerType*>(rtype);
        int size = std::max(lhsInt->size, rhsInt->size);
        bool isSigned = lhsInt->isSigned || rhsInt->isSigned;
        //now look up the integer type with given size and signedness
        this->type = getIntegerType(size, isSigned);
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
        auto lhsFloat = dynamic_cast<FloatType*>(ltype);
        auto rhsFloat = dynamic_cast<FloatType*>(rtype);
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
    type = primitives[Parser::TypeNT::ULONG];
  }
  else
  {
    type = primitives[Parser::TypeNT::UINT];
  }
}

FloatLiteral::FloatLiteral(FloatLit* a) : Expression(NULL), value(a->val)
{
  type = primitives[Parser::TypeNT::DOUBLE];
}

FloatLiteral::FloatLiteral(double val) : Expression(NULL), value(val)
{
  type = primitives[Parser::TypeNT::DOUBLE];
}

StringLiteral::StringLiteral(StrLit* a) : Expression(NULL)
{
  value = a->val;
  type = primitives[Parser::TypeNT::CHAR]->getArrayType(1);
}

CharLiteral::CharLiteral(CharLit* a) : Expression(NULL)
{
  value = a->val;
  type = primitives[Parser::TypeNT::CHAR];
}

BoolLiteral::BoolLiteral(Parser::BoolLit* a) : Expression(NULL)
{
  value = a->val;
  type = primitives[Parser::TypeNT::BOOL];
}

/*******************
 * CompoundLiteral *
 *******************/

CompoundLiteral::CompoundLiteral(Scope* s, Parser::StructLit* a) : Expression(s)
{
  this->ast = a;
  //type cannot be determined for a compound literal
  type = NULL;
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
}

/***********
 * Indexed *
 ***********/

Indexed::Indexed(Scope* s, Parser::Expr12::ArrayIndex* a) : Expression(s)
{
  //get expressions for the index and the indexed object
  group = getExpression(s, a->arr);
  index = getExpression(s, a->index);
  semanticCheck();
}

Indexed::Indexed(Scope* s, Expression* grp, Expression* ind) : Expression(s)
{
  group = grp;
  index = ind;
  semanticCheck();
}

void Indexed::semanticCheck()
{
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

/******************
 * SubroutineExpr *
 ******************/

SubroutineExpr::SubroutineExpr(Scope* scope, Subroutine* s) : Expression(s)
{
  this->subr = s;
  //expr type is the callable type for subr
  bool pure = s->isPure();
  Type* returnType = s->retType; 
  vector<Type*>& args = s->argTypes;
  //type lookup will always succeed, because the subroutine exists already
  //so its return/argument types have already been checked
  type = CallableType::lookup(pure, s->isStatic, returnType, args);
}

/*************
 * StructMem *
 *************/

StructMem::StructMem(Scope* s, Expression* base, vector<string>& names)
{
  //recursively walk members for all names
  //at the same time, make sure the name is actually a data member
  StructType* st = dynamic_cast<StructType*>(base->type);
  if(!st)
  {
    ERR_MSG("error: type " << base->type->getName() << " is not a struct, so cannot access any named members");
  }
  for(size_t i = 0; i < names.size(); i++)
  {
    string& name = names[i];
    bool foundName = false;
    for(size_t j = 0; j < st->memberNames.size(); j++)
    {
      if(st->memberNames[i] == name)
      {
        memberIndices.push_back(j);
        foundName = true;
        break;
      }
    }
    if(!foundName)
    {
      ERR_MSG("type " << st->getName() << " has no member named " << name);
    }
    else if(i != names.size() - 1)
    {
      //not the last name in chain, so find the next struct type
      Type* next = st->members[memberIndices.back()];
      st = dynamic_cast<StructType*>(next);
      if(!st)
      {
        ERR_MSG("type " << next->getName() << " is not a struct, so cannot access any named members");
      }
    }
  }
}

/************
 * NewArray *
 ************/

NewArray::NewArray(Scope* s, Parser::NewArrayNT* ast) : Expression(s)
{
  auto elemType = lookupType(ast->elemType, s);
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

/***************
 * ArrayLength *
 ***************/

ArrayLength::ArrayLength(Scope* s, Expression* arr) : Expression(s)
{
  array = arr;
  this->type = Type::primitives[TypeNT::UINT];
}

/***********
 * TempVar *
 ***********/

TempVar::TempVar(string id, Type* t, Scope* s) : Expression(s), ident(id) {}

