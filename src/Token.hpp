#ifndef TOKEN_H
#define TOKEN_H

#include "Misc.hpp"
#include "Type.hpp"

#include <map>
#include <vector>

using namespace std;

extern map<string, int> keywordTable;
extern map<string, int> operatorTable;
extern map<char, int> punctTable;
extern vector<string> tokTypeTable;

void initTokens();

//return index in Keyword enum, or -1
int isKeyword(string str);

enum KW
{
  FUNC,
  PROC,
  VOID,
  BOOL,
  CHAR,
  UCHAR,
  SHORT,
  USHORT,
  INT,
  UINT,
  LONG,
  ULONG,
  STRING,
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
  CASE,
  DEFAULT,
  BREAK,
  CONTINUE,
  METAIF,
  METAELSE,
  METAFOR,
  METAFUNC,
  VARIANT,
  AUTO,
  MODULE,
  USING,
  TRUE,
  FALSE,
  FUNCTYPE,
  PROCTYPE,
  NONTERM
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

enum PUNC
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
  DOLLAR
};

enum TokType
{
  IDENTIFIER,
  STRING_LITERAL,
  CHAR_LITERAL,
  INT_LITERAL,
  FLOAT_LITERAL,
  PUNCTUATION,
  OPERATOR,
  KEYWORD,
  PAST_EOF          //null or empty token
  NUM_TOKEN_TYPES
};

struct Token
{
  virtual bool operator==(const Token& rhs) = 0;
  virtual int getType() = 0;
  virtual string getStr() = 0;    //string equal to (or at least describing) token for error messages
  virtual string getDesc() = 0;   //get description of the token type, i.e. "identifier" or "operator"
};

//Identifier: variable name or type name
struct Ident : public Token
{
  Ident(string name);
  bool operator==(const Ident& rhs);
  int getType();
  string getStr();
  string getDesc();
  string name;
};

//Operator: non-structure punctuation sequence
struct Oper : public Token
{
  Oper(int op);
  bool operator==(const Oper& rhs);
  int getType();
  string getStr();
  string getDesc();
  int op;
};

//"xyz"
struct StrLit : public Token
{
  StrLit(string val);
  bool operator==(const StrLit& rhs);
  int getType();
  string getStr();
  string getDesc();
  string val;
};

//'?'
struct CharLit : public Token
{
  CharLit(char val);
  bool operator==(const CharLit& rhs);
  int getType();
  string getStr();
  string getDesc();
  char val;
};

//[-]0123456789
struct IntLit : public Token
{
  IntLit(int val);
  bool operator==(const IntLit& rhs);
  int getType();
  string getStr();
  string getDesc();
  int val;
};

//any float/double
struct FloatLit : public Token
{
  FloatLit(double val);
  bool operator==(const FloatLit& rhs);
  int getType();
  string getStr();
  string getDesc();
  double val;
};

//Structure punctuation: (){};,.
struct Punct : public Token
{
  Punct(PUNC val);
  bool operator==(const Punct& rhs);
  int getType();
  string getStr();
  string getDesc();
  int val;
};

struct Keyword : public Token
{
  Keyword(string text);
  Keyword(int val);
  bool operator==(const Keyword& rhs);
  int getType();
  string getStr();
  string getDesc();
  int kw;
};

struct PastEOF : public Token
{
  PastEOF();
  static PastEOF inst;
  bool operator==(const Keyword& rhs);
  int getType();
  string getStr();
  string getDesc();
};

#endif

