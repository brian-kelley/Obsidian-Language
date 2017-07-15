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
  leaves.push_back(getExpression(s, expr->head));
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
    BinaryArith* chain = new BinaryArith(leaves[0], BinaryOp::LOR, leaves[1]);
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
  leaves.push_back(getExpression(s, expr->head));
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
  leaves.push_back(getExpression(s, expr->head));
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
  leaves.push_back(getExpression(s, expr->head));
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
  leaves.push_back(getExpression(s, expr->head));
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
  leaves.push_back(getExpression(s, expr->head));
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
  leaves.push_back(getExpression(s, expr->head));
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
  leaves.push_back(getExpression(s, expr->head));
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
  leaves.push_back(getExpression(s, expr->head));
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
Expression* getExpression<Parser::Expr11>(Parser::Expr11* expr)
{
  if(expr->e.is<AP(Parser::Expr12))
  {
    return getExpression(s, expr->e.get<AP(Parser::Expr12)>().get());
  }
  else
  {
    //unary expression, with a single Expr11 as the operand
    auto unary = expr->e.get<Parser::Expr11::UnaryExpr>();
    Expression* operand = getExpression(s, unary->rhs.get());
    return new UnaryArith(unary->op, operand);
  }
}

template<>
Expression* getExpression<Parser::Expr12>(Parser::Expr12* expr)
{
  if(expr->e.is<IntLit*>())
  {
    return 
  }
  /*
   * Expr12:
  {
    struct ArrayIndex
    {
      //arr[index]
      AP(Expr12) arr;
      AP(ExpressionNT) index;
    };
    variant<
      None,
      IntLit*,
      CharLit*,
      StrLit*,
      FloatLit*,
      AP(BoolLit),
      AP(ExpressionNT),
      AP(Member),
      AP(StructLit),
      AP(TupleLit),
      AP(Call),
      ArrayIndex> e;
  }; 
  */
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

UnaryArith::UnaryArith(Scope* s, Parser::Expr11* ast) : Expression(s)
{
  this->ast = ast;
}

/***************
 * BinaryArith *
 ***************/

BinaryArith::BinaryArith(Scope* s, Parser::Expr1* ast) : Expression(s)
{
}

BinaryArith::BinaryArith(Scope* s, Parser::Expr2* ast) : Expression(s)
{
}

BinaryArith::BinaryArith(Scope* s, Parser::Expr3* ast) : Expression(s)
{
}

BinaryArith::BinaryArith(Scope* s, Parser::Expr4* ast) : Expression(s)
{
}

BinaryArith::BinaryArith(Scope* s, Parser::Expr5* ast) : Expression(s)
{
}

BinaryArith::BinaryArith(Scope* s, Parser::Expr6* ast) : Expression(s)
{
}

BinaryArith::BinaryArith(Scope* s, Parser::Expr7* ast) : Expression(s)
{
}

BinaryArith::BinaryArith(Scope* s, Parser::Expr8* ast) : Expression(s)
{
}

BinaryArith::BinaryArith(Scope* s, Parser::Expr9* ast) : Expression(s)
{
}

BinaryArith::BinaryArith(Scope* s, Parser::Expr10* ast) : Expression(s)
{
}

/********************
 * PrimitiveLiteral *
 ********************/

PrimitiveLiteral::PrimitiveLiteral(Scope* s, Parser::Expr12* ast) : Expression(s)
{
}

/*******************
 * CompoundLiteral *
 *******************/

CompoundLiteral::CompoundLiteral(Scope* s, Parser::StructLit* ast) : Expression(s)
{
}

/***********
 * Indexed *
 ***********/

Indexed::Indexed(Scope* s, Parser::Expr12::ArrayIndex* ast) : Expression(s)
{
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
}

}
