#include "Common.hpp"

namespace LexerFuzzing
{

//Get a random byte - about 2/3 likely to be
//some ASCII control character which the lexer should reject
char getRandomByte()
{
  return rand() & 0xFF;
}

//Get a random "normal" ASCII chacter: alphanumeric/space/punctuation
char getRandomASCII()
{
  //Get a normal ASCII character between ' ' and '~'
  return ' ' + (rand() % ('~' - ' '));
}

string genRandomFile(size_t len, bool allowSpecial)
{
  string fname = "lexfuzz.os";
  ofstream f(fname);
  for(size_t i = 0; i < len; i++)
  {
    if(allowSpecial)
      f << getRandomByte();
    else
      f << getRandomASCII();
  }
  f.close();
  return fname;
}

}

