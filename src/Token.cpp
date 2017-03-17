#include "Token.hpp"

int isKeyword(string str)
{
  if(str == "void") return VOID;
  if(str == "bool") return BOOL;
  if(str == "char") return CHAR;
  if(str == "uchar") return UCHAR;
  if(str == "short") return SHORT;
  if(str == "ushort") return USHORT;
  if(str == "int") return INT;
  if(str == "uint") return UINT;
  if(str == "long") return LONG;
  if(str == "ulong") return ULONG;
  if(str == "string") return STRING;
  if(str == "float") return FLOAT;
  if(str == "double") return DOUBLE;
  if(str == "print") return PRINT;
  if(str == "return") return RETURN;
  if(str == "typedef") return TYPEDEF;
  if(str == "struct") return STRUCT;
  if(str == "func") return FUNC;
  if(str == "proc") return PROC;
  if(str == "error") return ERROR;
  if(str == "trait") return TRAIT;
  if(str == "if") return IF;
  if(str == "else") return ELSE;
  if(str == "for") return FOR;
  if(str == "while") return WHILE;
  if(str == "switch") return SWITCH;
  if(str == "case") return CASE;
  if(str == "default") return DEFAULT;
  if(str == "break") return BREAK;
  if(str == "continue") return CONTINUE;
  if(str == "metaif") return METAIF;
  if(str == "metaelse") return METAELSE;
  if(str == "metafor") return METAFOR;
  if(str == "metafunc") return METAFUNC;
  if(str == "variant") return VARIANT;
  if(str == "auto") return AUTO;
  if(str == "module") return MODULE;
  if(str == "enum") return ENUM;
  if(str == "using") return USING;
  if(str == "true") return TRUE;
  if(str == "false") return FALSE;
  if(str == "assert") return ASSERT;
  if(str == "test") return TEST;
  if(str == "functype") return FUNCTYPE;
  if(str == "proctype") return PROCTYPE;
  if(str == "nonterm") return NONTERM;
  return -1;
}

/* Identifier */
Ident::Ident(string name)
{
  this->name = name;
}

/* Operator */
Oper::Oper(int op)
{
  this->op = op;
}

/* String Literal */
StrLit::StrLit(string val)
{
  this->val = val;
}

/* Character Literal */
CharLit::CharLit(char val)
{
  this->val = val;
}

/* Integer Literal */
IntLit::IntLit(int val)
{
  this->val = val;
}

/* Punctuation */
Punct::Punct(PUNC val)
{
  this->val = val;
}

/* Keyword */
Keyword::Keyword(string text)
{
  int val = isKeyword(text);
  if(val == -1)
  {
    errAndQuit("Expected a keyword.");
  }
  this->kw = val;
}

Keyword::Keyword(int val)
{
  this->kw = val;
}

