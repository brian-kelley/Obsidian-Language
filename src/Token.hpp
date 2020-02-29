#ifndef TOKEN_H
#define TOKEN_H

#include "Common.hpp"
#include "AST.hpp"

void initTokens();
void setOperatorPrec();

/* Token enum declarations */

enum KeywordEnum
{
  FUNC,
  PROC,
  VOID,
  ERROR,
  BOOL,
  CHAR,
  BYTE,
  UBYTE,
  SHORT,
  USHORT,
  INT,
  UINT,
  LONG,
  ULONG,
  FLOAT,
  DOUBLE,
  PRINT,
  RETURN,
  TYPEDEF,
  STRUCT,
  THIS,
  IF,
  ELSE,
  FOR,
  WHILE,
  SWITCH,
  MATCH,
  CASE,
  DEFAULT,
  BREAK,
  CONTINUE,
  TYPE,
  ENUM,
  AUTO,
  MODULE,
  USING,
  TRUE,
  FALSE,
  FUNCTYPE,
  PROCTYPE,
  IS,
  AS,
  TEST,
  BENCHMARK,
  ASSERT,
  STATIC,
  ARRAY,
  EXTERN,
  CONST,
  NUM_KEYWORDS,
  INVALID_KEYWORD
};

enum OperatorEnum
{
  PLUS,
  PLUSEQ,
  SUB,
  SUBEQ,
  MUL,
  MULEQ,
  DIV,
  DIVEQ,
  MOD,
  MODEQ,
  LOR,
  BOR,
  BOREQ,
  BXOR,
  BXOREQ,
  LNOT,
  BNOT,
  LAND,
  BAND,
  BANDEQ,
  SHL,
  SHLEQ,
  SHR,
  SHREQ,
  CMPEQ,
  CMPNEQ,
  CMPL,
  CMPLE,
  CMPG,
  CMPGE,
  ASSIGN,
  INC,
  DEC,
  ARROW,
  INVALID_OPERATOR
};

enum PunctEnum
{
  COMMA,
  SEMICOLON,
  COLON,
  DOT,
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  LBRACKET,
  RBRACKET,
  BACKSLASH,
  QUESTION,
  DOLLAR,
  HASH,
  INVALID_PUNCT
};

enum TokenTypeEnum
{
  IDENTIFIER,
  STRING_LITERAL,
  CHAR_LITERAL,
  INT_LITERAL,
  FLOAT_LITERAL,
  PUNCTUATION,
  OPERATOR,
  KEYWORD,
  PAST_EOF,         //null or empty token
  NUM_TOKEN_TYPES,
  INVALID_TOKEN_TYPE
};

/* Token types */

struct Token : public Node
{
  Token();
  virtual bool compareTo(Token* rhs) = 0;
  virtual string getStr() = 0;    //string equivalent to original text
  TokenTypeEnum type;
};

//Identifier: variable name or type name
struct Ident : public Token
{
  Ident();
  Ident(string name);
  bool compareTo(Token* rhs);
  bool operator==(Ident& rhs);
  string getStr();
  string name;
};

//Operator: non-structure punctuation sequence
struct Oper : public Token
{
  Oper();
  Oper(OperatorEnum op);
  bool compareTo(Token* rhs);
  bool operator==(Oper& rhs);
  string getStr();
  OperatorEnum op;
};

//"xyz"
struct StrLit : public Token
{
  StrLit();
  StrLit(string val);
  bool compareTo(Token* rhs);
  bool operator==(StrLit& rhs);
  string getStr();
  string val;
};

//'?'
struct CharLit : public Token
{
  CharLit();
  CharLit(char val);
  bool compareTo(Token* rhs);
  bool operator==(CharLit& rhs);
  string getStr();
  char val;
};

//[-]{0123456789}+
struct IntLit : public Token
{
  IntLit();
  IntLit(uint64_t val);
  bool compareTo(Token* rhs);
  bool operator==(IntLit& rhs);
  string getStr();
  //note: val is always positive because any minus sign is read in as operator -
  uint64_t val;
};

//any float/double
struct FloatLit : public Token
{
  FloatLit();
  FloatLit(double val);
  bool compareTo(Token* rhs);
  bool operator==(FloatLit& rhs);
  string getStr();
  //note: val is always positive
  double val;
};

//Structure punctuation: (){};,.
struct Punct : public Token
{
  Punct();
  Punct(PunctEnum val);
  bool compareTo(Token* rhs);
  bool operator==(Punct& rhs);
  string getStr();
  PunctEnum val;
};

struct Keyword : public Token
{
  Keyword();
  Keyword(string text);
  Keyword(KeywordEnum val);
  bool compareTo(Token* rhs);
  bool operator==(Keyword& rhs);
  string getStr();
  KeywordEnum kw;
};

struct PastEOF : public Token
{
  PastEOF();
  static PastEOF inst;
  bool compareTo(Token* rhs);
  bool operator==(PastEOF& rhs);
  string getStr();
};

/* Utility functions */

KeywordEnum getKeyword(const string& str);
PunctEnum getPunct(char c);
OperatorEnum getOper(const string& str);
bool isOperCommutative(OperatorEnum o);
int getOperPrecedence(OperatorEnum o);
string getTokenTypeDesc(TokenTypeEnum tte);
string getTokenTypeDesc(Token* t);

#endif

