#include "Preprocessor.hpp"

void stripComments(Text& t)
{
  char* newBuf = new char[t.size];
  for(size_t i = 0; i < t.size; i++)
  {
    if(t.buf[i] == '\"')
    {
      //Found string literal, scan ahead to the next non-escaped "
      size_t j;
      for(j = i; j < t.size; j++)
      {
        if(t.buf[j] == 
      }
      if(j == t.size)
      {
        puts("Error: non-terminated string literal.");
        exit(EXIT_FAILURE);
      }

    }
  }
  delete[] t.buf;
  t.buf = newBuf;
}
