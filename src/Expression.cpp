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
    root = memberToExpression(expr->e.get<Parser::Member*>(), s);
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
  vector<string> rhsNames;
  for(size_t i = 0; i < expr->tail.size(); i++)
  {
    //apply each rhs to e12 to get the final expression
    //consective names are handled in a group by applyNamesToExpr12,
    //so store the names in rhsNames as they are encountered
    //Call and index operators are handled here, one at a time
    auto& e12rhs = expr->tail[i]->e;
    if(e12rhs.is<Ident*>())
    {
      //special case: len as a member of array
      string& id = e12rhs.get<Ident*>()->name;
      if(id == "len" && dynamic_cast<ArrayType*>(root->type))
      {
        root = new ArrayLength(s, root);
      }
      else
      {
        rhsNames.push_back(id);
      }
      continue;
    }
    if(rhsNames.size())
    {
      applyNamesToExpr12(root, rhsNames);
      rhsNames.clear();
    }
    if(e12rhs.is<Parser::CallOp*>())
    {
      vector<Expression*> args;
      auto co = e12rhs.get<Parser::CallOp*>();
      for(auto arg : co->args)
      {
        args.push_back(getExpression(s, arg));
      }
      root = new CallExpr(s, root, args);
    }
    else if(e12rhs.is<Parser::ExpressionNT*>())
    {
      //array index
      Expression* index = getExpression(s, e12rhs.get<Parser::ExpressionNT*>());
      root = new Indexed(s, root, index);
    }
    else
    {
      //probably something wrong with parser
      INTERNAL_ERROR;
    }
  }
  //apply all remaining names
  applyNamesToExpr12(root, rhsNames);
  return root;
}

Expression* memberToExpression(Parser::Member* mem, Scope* s)
{
  //look up one name at a time
  vector<Name> names;
  Scope* iter = s;
}

Expression* applyExpr12RHS(Scope* s, Expression* root, Parser::Expr12RHS* e12)
{
  if(e12->e.is<Parser::CallOp*>())
  {
    //method call
    //make sure that root is Callable
    auto ct = dynamic_cast<CallableType*>(root->type);
    if(!ct)
    {
      ERR_MSG("tried to call a non-callable member");
    }
    auto co = e12->e.get<Parser::CallOp*>();
    vector<Expression*> args;
    for(auto arg : co->args)
    {
      args.push_back(getExpression(s, arg));
    }
    return new CallExpr(s, root, args);
  }
  else if(e12->e.is<Parser::ExpressionNT*>())
  {
    return new Indexed(s, root, getExpression(s, e12->e.get<Parser::ExpressionNT*>()));
  }
  else
  {
    //didn't handle Ident* properly in getExpression<Expr12>
    INTERNAL_ERROR;
  }
}

Expression* applyNamesToExpr12(Expression* root, vector<string>& names)
{
  Scope* search = scopeForExpr(root);
  //will search for names within scope corresponding to st
  //copy names to itemNames so struct members can be found
  vector<string> itemNames;
  for(size_t i = 0; i < names.size(); i++)
  {
    string& name = names[i];
    auto it = search->names.find(name);
    if(it == search->names.end())
    {
      ERR_MSG("no member " << name << " in scope " << search->getFullPath());
    }
    Name& n = it->second;
    switch(n.type)
    {
      case Name::SCOPE:
        //if this is the last name, error
        if(i == names.size() - 1)
        {
          ERR_MSG("member " << name << " of scope " << search->getFullPath() << " is a scope, not an expression");
        }
        //continue the search from this scope
        search = (Scope*) n.item;
        itemNames.push_back(name);
        break;
      case Name::VARIABLE:
      {
        //set root to the struct mem
        //variable must be non-static
        Variable* v = (Variable*) n.item;
        if(v->isStatic)
        {
          ERR_MSG("can't access static variable " << v->name << " through an object");
        }
        root = new StructMem(root->scope, root, v);
        if(i == names.size() - 1)
          return root;
        //have more names after this, so need to continue search from new struct scope
        itemNames.clear();
        search = scopeForExpr(root);
        break;
      }
      case Name::SUBROUTINE:
        root = new SubroutineExpr(root->scope, root, (Subroutine*) n.item);
        if(i == names.size() - 1)
          return root;
        //not the last name, but it's impossible for a subroutine to have members
        ERR_MSG("function or procedure has no members");
        break;
      case Name::ENUM:
        //TODO
        INTERNAL_ERROR;
        break;
      default: ERR_MSG("member " << name << " of scope " << search->getFullPath() << " is not an expression or scope");
    }
  }
  return NULL;
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
      //TODO (CTE): catch div by 0
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

Indexed::Indexed(Scope* s, Expression* grp, Expression* ind) : Expression(s)
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
  else
  {
    ERR_MSG("expression can't be subscripted.");
  }
}

/************
 * CallExpr *
 ************/

CallExpr::CallExpr(Scope* s, Expression* c, vector<Expression*>& a) : Expression(s)
{
  //callable expressions should only be produced from Expr12, so callable actually
  //being a Callable must already have been checked
  if(!dynamic_cast<CallableType*>(c->type))
    INTERNAL_ERROR;
  callable = c;
  args = a;
  checkArgs((CallableType*) callable->type, args);
}

void checkArgs(CallableType* callable, vector<Expression*>& args)
{
  //make sure number of arguments matches
  if(callable->argTypes.size() != args.size())
  {
    ERR_MSG("in call to " << (callable->isStatic ? "static " : "") <<
        (callable->pure ? "function" : "procedure") << ", expected " <<
        callable->argTypes.size() << " arguments but got " << args.size());
  }
  for(size_t i = 0; i < args.size(); i++)
  {
    //make sure arg value can be converted to expected type
    if(!callable->argTypes[i]->canConvert(args[i]))
    {
      ERR_MSG("argument " << i + 1 << " to " << (callable->isStatic ? "static " : "") <<
        (callable->pure ? "function" : "procedure") << " has wrong type (expected " <<
        callable->argTypes[i]->getName() << " but got " <<
        (args[i]->type ? args[i]->type->getName() : "incompatible compound literal") << ")");
    }
  }
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

SubroutineExpr::SubroutineExpr(Scope* scope, Subroutine* s) : Expression(scope)
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

StructMem::StructMem(Scope* s, Expression* b, Variable* v) : Expression(s)
{
  base = b;
  member = v;
  type = v->type;
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
  this->type = primitives[Parser::TypeNT::UINT];
}

/***********
 * TempVar *
 ***********/

TempVar::TempVar(string id, Type* t, Scope* s) : Expression(s), ident(id) {}

