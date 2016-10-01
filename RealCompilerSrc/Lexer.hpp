#ifndef LEXER_H
#define LEXER_H

#include "Misc.hpp"
#include "Token.hpp"

using namespace std;

namespace Lexer
{
  vector<Token> tokenize(char* buf);
}

#endif
