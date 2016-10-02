#include "Token.hpp"

int isKeyword(string str)
{
  if(str == "void") return 0;
  if(str == "char") return 1;
  if(str == "uchar") return 2;
  if(str == "short") return 3;
  if(str == "ushort") return 4;
  if(str == "int") return 5;
  if(str == "uint") return 6;
  if(str == "long") return 7;
  if(str == "ulong") return 8;
  if(str == "print") return 9;
  if(str == "return") return 10;
  if(str == "typedef") return 11;
  if(str == "struct") return 12;
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
