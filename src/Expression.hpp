#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "AST.hpp"

struct Expression : public Node
{
  Expression() : type(nullptr) {}
  virtual void resolve(bool err) {}
  TypeSystem::Type* type;
  //whether this works as an lvalue
  virtual bool assignable() = 0;
};

//Resolve an expression in-place
//this is needed because Expression::resolve() can't
//help if resolved expr has different subclass than unresolved
Expression* resolveExpr(Expression*& expr);

//Subclasses of Expression
struct UnaryArith;
struct BinaryArith;
struct IntLiteral;
struct FloatLiteral;
struct StringLiteral;
struct CharLiteral;
struct BoolLiteral;
struct CompoundLiteral;
struct Indexed;
struct CallExpr;
struct VarExpr;
struct NewArray;
struct Converted;
struct ArrayLength;
struct ThisExpr;
struct ErrorVal;
struct UnresolvedExpr;

//Create a new Expression given one of the ExprN nonterminals
template<typename NT>
Expression* getExpression(Scope* s, NT* expr);

//process one name as part of Expr12 or Expr12RHS
//returns true iff root is a valid expression that uses all names
void processExpr12Name(string name, bool& isFinal, bool first, Expression*& root, Scope*& scope);

//Assuming expr is a struct type, get the struct scope
//Otherwise, display relevant errors
StructScope* scopeForExpr(Expression* expr);

struct UnaryArith : public Expression
{
  UnaryArith(int op, Expression* expr);
  int op;
  Expression* expr;
  bool assignable()
  {
    return false;
  }
  void resolve(bool err);
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
  void resolve(bool err);
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
  private:
  void setType(); //called by both constructors
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
  string value;
  bool assignable()
  {
    return false;
  }
};

struct CharLiteral : public Expression
{
  CharLiteral(CharLit* ast);
  char value;
  bool assignable()
  {
    return false;
  }
};

struct BoolLiteral : public Expression
{
  BoolLiteral(Parser::BoolLit* ast);
  bool value;
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
  bool assignable()
  {
    return lvalue;
  }
  vector<Expression*> members;
  bool lvalue;
  void resolve(bool err);
};

struct Indexed : public Expression
{
  Indexed(Expression* grp, Expression* ind);
  Expression* group; //the array or tuple being subscripted
  Expression* index;
  bool assignable()
  {
    return group->assignable();
  }
  private:
  void semanticCheck(); //called by both constructors
};

struct CallExpr : public Expression
{
  CallExpr(Expression* callable, vector<Expression*>& args);
  Expression* callable;
  vector<Expression*> args;
  bool assignable()
  {
    return false;
  }
};

//helper to verify argument number and types
void checkArgs(TypeSystem::CallableType* callable, vector<Expression*>& args);

struct NamedExpr : public Expression
{
  NamedExpr(Parser::Member* name, Scope* s);
  NamedExpr(Variable* v);
  NamedExpr(Subroutine* s);
  NamedExpr(ExternalSubroutine* ex);
  variant<Variable*, Subroutine*, ExternalSubroutine*> value;
  Parser::Member* name;
  Scope* usage;
  bool assignable()
  {
    if(value.is<Variable*>())
      return true;
    else
      return false;
  }
  void resolve(bool err);
};

struct StructMem : public Expression
{
  StructMem(Expression* base, Variable* v);
  Expression* base;           //base->type is always a StructType
  Parser::Member* member;
  variant<Variable*, Subroutine*> member;           //member must be a member of base->type
  bool assignable()
  {
    return base->assignable();
  }
};

struct NewArray : public Expression
{
  NewArray(Type* elemType, vector<Expression*> dims);
  vector<Expression*> dims;
  bool assignable()
  {
    return false;
  }
};

struct ArrayLength : public Expression
{
  ArrayLength(Expression* arr);
  Expression* array;
  bool assignable()
  {
    return false;
  }
};

struct ThisExpr : public Expression
{
  ThisExpr(Scope* where);
  //structType == (StructType*) type,
  //structType is only for convenience
  TypeSystem::StructType* structType;
  bool assignable()
  {
    return true;
  }
};

struct Converted : public Expression
{
  Converted(Expression* val, TypeSystem::Type* dst);
  Expression* value;
  bool assignable()
  {
    return value->assignable();
  }
};

struct EnumExpr : public Expression
{
  EnumExpr(TypeSystem::EnumConstant* ec);
  int64_t value;
  bool assignable()
  {
    return false;
  }
};

struct ErrorVal : public Expression
{
  ErrorVal();
  bool assignable()
  {
    return false;
  }
};

void resolveExpr(Expression*& expr, bool err);

#endif

