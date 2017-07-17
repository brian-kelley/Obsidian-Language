#include "Expression.hpp"

namespace MiddleEndExpr
{

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
  leaves.push_back(getExpression(s, expr->head.get()));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs.get()));
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
  leaves.push_back(getExpression(s, expr->head.get()));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs.get()));
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
  leaves.push_back(getExpression(s, expr->head.get()));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs.get()));
  }
  if(leaves.size() == 1)
  {
    return leaves.front();
  }
  else
  {
    //build chain of BinaryAriths that evaluates left to right
    BinaryArith* chain = new BinaryArith(leaves[0], BOR, leaves[1]);
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
  leaves.push_back(getExpression(s, expr->head.get()));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs.get()));
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
  leaves.push_back(getExpression(s, expr->head.get()));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs.get()));
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
  leaves.push_back(getExpression(s, expr->head.get()));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs.get()));
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
  leaves.push_back(getExpression(s, expr->head.get()));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs.get()));
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
  leaves.push_back(getExpression(s, expr->head.get()));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs.get()));
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
  leaves.push_back(getExpression(s, expr->head.get()));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs.get()));
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
  leaves.push_back(getExpression(s, expr->head.get()));
  for(auto e : expr->tail)
  {
    leaves.push_back(getExpression(s, e->rhs.get()));
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
  if(expr->e.is<AP(Parser::Expr12)>())
  {
    return getExpression(s, expr->e.get<AP(Parser::Expr12)>().get());
  }
  else
  {
    //unary expression, with a single Expr11 as the operand
    auto unary = expr->e.get<Parser::Expr11::UnaryExpr>();
    Expression* operand = getExpression(s, unary.rhs.get());
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
  else if(expr->e.is<CharLit*>())
  {
    return new CharLiteral(expr->e.get<CharLit*>());
  }
  else if(expr->e.is<StrLit*>())
  {
    return new StringLiteral(expr->e.get<StrLit*>());
  }
  else if(expr->e.is<AP(Parser::BoolLit)>())
  {
    return new BoolLiteral(expr->e.get<AP(Parser::BoolLit)>().get());
  }
  else if(expr->e.is<AP(Parser::ExpressionNT)>())
  {
    return getExpression(s, expr->e.get<AP(Parser::ExpressionNT)>().get());
  }
  else if(expr->e.is<AP(Parser::Member)>())
  {
    return new Var(s, expr->e.get<AP(Parser::Member)>().get());
  }
  else if(expr->e.is<AP(Parser::StructLit)>())
  {
    return new CompoundLiteral(s, expr->e.get<AP(Parser::StructLit)>().get());
  }
  else if(expr->e.is<AP(Parser::TupleLit)>())
  {
    return new TupleLiteral(s, expr->e.get<AP(Parser::TupleLit)>().get());
  }
  else if(expr->e.is<AP(Parser::Call)>())
  {
    return new Call(s, expr->e.get<AP(Parser::Call)>().get());
  }
  else if(expr->e.is<Parser::Expr12::ArrayIndex>())
  {
    return new Indexed(s, &(expr->e.get<Parser::Expr12::ArrayIndex>()));
  }
  else
  {
    INTERNAL_ERROR;
    return nullptr;
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

UnaryArith::UnaryArith(int op, Expression* expr) : Expression(nullptr)
{
}

/***************
 * BinaryArith *
 ***************/

BinaryArith::BinaryArith(Expression* lhs, int op, Expression* rhs) : Expression(nullptr)
{
}

/**********************
 * Primitive Literals *
 **********************/

IntLiteral::IntLiteral(IntLit* ast) : Expression(nullptr)
{
  this->ast = ast;
  //if value fits in a signed int, use that as the type
  //when in doubt, don't use auto
  if(value() > 0x7FFFFFFF)
  {
    type = TypeSystem::primitives[Parser::TypeNT::ULONG];
  }
  else
  {
    type = TypeSystem::primitives[Parser::TypeNT::UINT];
  }
}

FloatLiteral::FloatLiteral(FloatLit* ast) : Expression(nullptr)
{
  this->ast = ast;
  type = TypeSystem::primitives[Parser::TypeNT::DOUBLE];
}

StringLiteral::StringLiteral(StrLit* ast) : Expression(nullptr)
{
  this->ast = ast;
  type = TypeSystem::primitives[Parser::TypeNT::STRING];
}

CharLiteral::CharLiteral(CharLit* ast) : Expression(nullptr)
{
  this->ast = ast;
  type = TypeSystem::primitives[Parser::TypeNT::CHAR];
}

BoolLiteral::BoolLiteral(Parser::BoolLit* ast) : Expression(nullptr)
{
  this->ast = ast;
  type = TypeSystem::primitives[Parser::TypeNT::BOOL];
}

/*******************
 * CompoundLiteral *
 *******************/

CompoundLiteral::CompoundLiteral(Scope* s, Parser::StructLit* ast) : Expression(s)
{
  this->ast = ast;
  //type cannot be determined for a compound literal
  type = nullptr;
  for(auto v : ast->vals)
  {
    //add member expression
    members.push_back(getExpression(s, v.get()));
  }
}

/****************
 * TupleLiteral *
 ****************/

TupleLiteral::TupleLiteral(Scope* s, Parser::TupleLit* ast) : Expression(s)
{
  this->ast = ast;
  vector<TypeSystem::Type*> memTypes;
  bool typeResolved = true;
  for(auto it : ast->vals)
  {
    members.push_back(getExpression(s, it.get()));
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

Indexed::Indexed(Scope* s, Parser::Expr12::ArrayIndex* ast) : Expression(s)
{
  this->ast = ast;
  //get expressions for the index and the indexed object
  group = getExpression(s, ast->arr.get());
  index = getExpression(s, ast->index.get());
  //Indexing a CompoundLiteral is not allowed at all
  //Indexing a Tuple (literal, variable or call) requires the index to be an IntLit
  //Anything else is assumed to be an array and then the index can be any integer expression
  if(dynamic_cast<CompoundLiteral*>(group))
  {
    errAndQuit("Can't index a compound literal - assign it to an array first.");
  }
  //note: ok if this is null
  auto tt = dynamic_cast<TypeSystem::TupleType*>(group->type);
  //in all other cases, group must have a type now
  if(tt)
  {
    //group's type is a Tuple, whether group is a literal, var or call
    //make sure the index is an IntLit
    auto intIndex = dynamic_cast<IntLiteral*>(index);
    if(intIndex)
    {
      //val is unsigned and so always positive
      auto val = intIndex->value();
      if(val >= tt->members.size())
      {
        errAndQuit(string("Tuple subscript out of bounds: tuple has ") + to_string(tt->members.size()) + " but requested member " + to_string(val));
      }
      type = tt->members[val];
    }
    else
    {
      errAndQuit("Tuple subscript must be an integer constant.");
    }
  }
  else
  {
    //group must be an array
    auto at = dynamic_cast<TypeSystem::ArrayType*>(group->type);
    if(at)
    {
      type = at->elem;
    }
    else
    {
      errAndQuit("Expression can't be subscripted.");
    }
  }
}

/********
 * Call *
 ********/

Call::Call(Scope* s, Parser::Call* ast) : Expression(s)
{
}

/*******
 * Var *
 *******/

Var::Var(Scope* s, Parser::Member* ast) : Expression(s)
{
  //To get type and Variable ptr, look up the variable in scope tree
}

}
