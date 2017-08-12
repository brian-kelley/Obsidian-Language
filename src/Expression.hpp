#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "variant.h"

struct Expression
{
  //Expression constructor will determine the type (implemented in subclasses)
  Expression(Scope* s);
  Scope* scope;
  //TODO: throw if any var decl with "auto" as type has an untyped RHS
  TypeSystem::Type* type;
  virtual bool assignable() = 0;
};

//Create a new Expression given one of the ExprN nonterminals
template<typename NT>
Expression* getExpression(Scope* s, NT* expr);

struct UnaryArith : public Expression
{
  //Precondition: ast->e is an Expr11::UnaryExpr
  UnaryArith(int op, Expression* expr);
  int op;
  Expression* expr;
  bool assignable()
  {
    return false;
  }
};

struct BinaryArith : public Expression
{
  BinaryArith(Expression* lhs, int op, Expression* rhs);
  int op;
  Expression* lhs;
  Expression* rhs;
  bool assignable()
  {
    return false;
  }
};

struct IntLiteral : public Expression
{
  IntLiteral(IntLit* ast);
  IntLiteral(uint64_t val);
  uint64_t value;
  bool assignable()
  {
    return false;
  }
  void setType();
};

struct FloatLiteral : public Expression
{
  FloatLiteral(FloatLit* ast);
  FloatLiteral(double val);
  double value;
  bool assignable()
  {
    return false;
  }
};

struct StringLiteral : public Expression
{
  StringLiteral(StrLit* ast);
  StrLit* ast;
  string value()
  {
    return ast->val;
  }
  bool assignable()
  {
    return false;
  }
};

struct CharLiteral : public Expression
{
  CharLiteral(CharLit* ast);
  CharLit* ast;
  char value()
  {
    return ast->val;
  }
  bool assignable()
  {
    return false;
  }
};

struct BoolLiteral : public Expression
{
  BoolLiteral(Parser::BoolLit* ast);
  Parser::BoolLit* ast;
  bool value()
  {
    return ast->val;
  }
  bool assignable()
  {
    return false;
  }
};

//it is impossible to determine the type of a CompoundLiteral by itself
//CompoundLiteral covers both array and struct literals
struct CompoundLiteral : public Expression
{
  CompoundLiteral(Scope* s, Parser::StructLit* ast);
  Parser::StructLit* ast;
  bool assignable()
  {
    return false;
  }
  vector<Expression*> members;
};

//Unlike CompoundLiteral, can get the type of TupleLiteral by itself
//as long as all members' types are known, so
//"auto tup = (1, 2, 3, 4.5, "hello");" is valid
struct TupleLiteral : public Expression
{
  TupleLiteral(Scope* s, Parser::TupleLit* ast);
  Parser::TupleLit* ast;
  bool assignable()
  {
    //a tuple expr is assignable as a whole only if all its members are
    for(auto m : members)
    {
      if(!m->assignable())
      {
        return false;
      }
    }
    return true;
  }
  vector<Expression*> members;
};

struct Indexed : public Expression
{
  Indexed(Scope* s, Parser::Expr12::ArrayIndex* ast);
  Expression* group; //the array or tuple being subscripted
  Expression* index;
  Parser::Expr12::ArrayIndex* ast;
  bool assignable()
  {
    return group->assignable();
  }
};

struct CallExpr : public Expression
{
  CallExpr(Scope* s, Parser::CallNT* ast);
  Subroutine* subr;
  bool assignable()
  {
    return false;
  }
};

struct VarExpr : public Expression
{
  VarExpr(Scope* s, Parser::Member* ast);
  VarExpr(Scope* s, Variable* v);
  Variable* var;  //var must be looked up from current scope
  bool assignable()
  {
    //all variables are lvalues (there is no const)
    return true;
  }
};

#endif

