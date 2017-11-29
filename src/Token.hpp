#ifndef TOKEN_H
#define TOKEN_H

#include "Common.hpp"
#include "PoolAlloc.hpp"

extern vector<string> keywordTable;
extern map<string, int> keywordMap;
extern vector<string> operatorTable;
extern map<string, int> operatorMap;
extern vector<char> punctTable;
extern map<char, int> punctMap;
extern vector<string> tokTypeTable;

void initTokens();

//return index in Keyword enum, or -1
int isKeyword(string str);

enum
{
  FUNC,
  PROC,
  VOID,
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
  ERROR,
  TRAIT,
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
  UNION,
  ENUM,
  AUTO,
  MODULE,
  USING,
  TRUE,
  FALSE,
  FUNCTYPE,
  PROCTYPE,
  NONTERM,
  TEST,
  BENCHMARK,
  ASSERT,
  STATIC,
  ARRAY
};

enum
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
  DEC
};

enum
{
  SEMICOLON,
  COLON,
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  LBRACKET,
  RBRACKET,
  DOT,
  COMMA,
  DOLLAR,
  QUESTION
};

enum
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
  NUM_TOKEN_TYPES
};

struct Token : public PoolAllocated
{
  Token();
  virtual bool compareTo(Token* rhs) = 0;
  virtual int getType() = 0;
  virtual string getStr() = 0;    //string equal to (or at least describing) token for error messages
  virtual string getDesc() = 0;   //get description of the token type, i.e. "identifier" or "operator"
  int type;
  int line;
  int col;
};

//Identifier: variable name or type name
struct Ident : public Token
{
  Ident();
  Ident(string name);
  bool compareTo(Token* rhs);
  bool operator==(Ident& rhs);
  int getType();
  string getStr();
  string getDesc();
  string name;
};

//Operator: non-structure punctuation sequence
struct Oper : public Token
{
  Oper();
  Oper(int op);
  bool compareTo(Token* rhs);
  bool operator==(Oper& rhs);
  int getType();
  string getStr();
  string getDesc();
  int op;
};

//"xyz"
struct StrLit : public Token
{
  StrLit();
  StrLit(string val);
  bool compareTo(Token* rhs);
  bool operator==(StrLit& rhs);
  int getType();
  string getStr();
  string getDesc();
  string val;
};

//'?'
struct CharLit : public Token
{
  CharLit();
  CharLit(char val);
  bool compareTo(Token* rhs);
  bool operator==(CharLit& rhs);
  int getType();
  string getStr();
  string getDesc();
  char val;
};

//[-]{0123456789}+
struct IntLit : public Token
{
  IntLit();
  IntLit(uint64_t val);
  bool compareTo(Token* rhs);
  bool operator==(IntLit& rhs);
  int getType();
  string getStr();
  string getDesc();
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
  int getType();
  string getStr();
  string getDesc();
  //note: val is always positive
  double val;
};

//Structure punctuation: (){};,.
struct Punct : public Token
{
  Punct();
  Punct(int val);
  bool compareTo(Token* rhs);
  bool operator==(Punct& rhs);
  int getType();
  string getStr();
  string getDesc();
  int val;
};

struct Keyword : public Token
{
  Keyword();
  Keyword(string text);
  Keyword(int val);
  bool compareTo(Token* rhs);
  bool operator==(Keyword& rhs);
  int getType();
  string getStr();
  string getDesc();
  int kw;
};

struct PastEOF : public Token
{
  PastEOF();
  static PastEOF inst;
  bool compareTo(Token* rhs);
  bool operator==(PastEOF& rhs);
  int getType();
  string getStr();
  string getDesc();
};

#endif

