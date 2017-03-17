#include "Misc.hpp"
#include "Token.hpp"
#include "Type.hpp"

typedef vector<Token*>::iterator TokIter;

//TokGroup: Any non-terminal node in the AST
//Use class hierarchy to represent the relationship between groups

struct Group
{
  vector<Group*> groups;
  vector<Token*> toks;
};

struct Expression : public Group
{
};

struct Statement : public Group
{
};

struct For : public Group
{
};

struct If : public Group
{
};

struct Program : public Group

struct AST
{
  AST::AST();
  Program p;
};

struct Function
{
  string name;
  Type* returnType;
  vector<TokGroup*> groups;
};

struct Program
{
  //Non-standard global type definitions
  vector<Type*> types;
  //Functions (including main)
  vector<Function*> funcs;
};

//Parse from a linear token stream into a structured program
AST parse(vector<Token*> toks);


