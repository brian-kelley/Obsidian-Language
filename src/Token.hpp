#ifndef TOKEN_H
#define TOKEN_H

#include "Misc.hpp"
#include "Type.hpp"

//return index in Keyword enum, or -1
int isKeyword(string str);

enum KW
{
  VOID,
  CHAR,
  UCHAR,
  SHORT,
  USHORT,
  INT,
  UINT,
  LONG,
  ULONG,
  PRINT,
  RETURN,
  TYPEDEF,
  STRUCT
};

enum OP
{
  PLUS,
  PLUSEQ,
  SUB,
  SUBEQ,
  MUL,
  MULEQ,
  DIV,
  DIVEQ,
  LOR,
  BOR,
  BXOR,
  LNOT,
  BNOT,
  LAND,
  BAND,
  SHL,
  SHR,
  CMPEQ,
  CMPNEQ,
  CMPL,
  CMPLE,
  CMPG,
  CMPGE,
  LBRACK,   //index operator, left and right
  RBRACK,
  ASSIGN
};

enum PUNC
{
  SEMICOLON,
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  DOT,
  COMMA
};

enum TokType
{
  IDENTIFIER,
  STRING_LITERAL,
  CHAR_LITERAL,
  INT_LITERAL,
  PUNCTUATION
};

struct Token
{
};

//Identifier: variable name or type name
struct Ident : public Token
{
  Ident(string name);
  string name;
};

//Operator: non-structure punctuation sequence
struct Oper : public Token
{
  Oper(int op);
  int op;
};

//"..."
struct StrLit : public Token
{
  StrLit(string val);
  string val;
};

//'.'
struct CharLit : public Token
{
  CharLit(char val);
  char val;
};

//0123456789
struct IntLit : public Token
{
  IntLit(int val);
  int val;
};

//Structure punctuation: (){};,.
struct Punct : public Token
{
  Punct(PUNC val);
  int val;
};

struct Keyword : public Token
{
  Keyword(string text);
  Keyword(int val);
  int kw;
};

#endif
