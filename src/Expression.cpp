#include "Expression.hpp"

Expression(Parser::Expression* expr, Scope* s)
{
  e = expr;
  scope = s;
  //find the type, based on variable context at this point in the scope
}

