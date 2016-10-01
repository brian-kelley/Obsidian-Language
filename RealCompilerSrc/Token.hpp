#ifndef TOKEN_H
#define TOKEN_H

#include "Misc.hpp"

enum TokType
{
  INTLIT,
  FLOATLIT,
  STRLIT,
  KEYWORD,
  VARNAME,
  OPERATOR,
}

enum Keyword
{
  INT,
  FLOAT,
  STRING,
  CHAR,
}

#endif

