#include "Preprocess.hpp"

void preprocess(string& t)
{
  stripComments(t);
}

void stripComments(string& t)
{
  string newT;
  newT.reserve(t.size());
  for(size_t i = 0; i < t.size() - 1; i++)
  {
    if(t[i] == '/' && t[i + 1] == '*')
    {
      //scan ahead and count the number of "/*" passed
      int depth = 1;
      i += 2;
      while(true)
      {
        if(i >= t.size() - 2)
        {
          errAndQuit("Error: unterminated /* comment */");
        }
        else if(t[i] == '/' && t[i + 1] == '*')
        {
          depth++;
          i += 2;
        }
        else if(t[i] == '*' && t[i + 1] == '/')
        {
          depth--;
          i += 2;
          if(depth == 0)
            break;
        }
        else
        {
          //eat a single character inside the comment
          i++;
        }
      }
    }
    else if(t[i] == '*' && t[i + 1] == '/')
    {
      errAndQuit("Error: */ without preceding /*");
    }
    else if(t[i] == '"')
    {
      while(true)
      {
        newT += t[i];
        i++;
        if(i == t.size())
        {
          errAndQuit("Error: unterminated string literal");
        }
        else if(t[i] == '"')
        {
          newT += t[i];
          i++;
          break;
        }
      }
    }
    else if(t[i] == '/' && t[i] == '/')
    {
      //guaranteed that there is a '\n' at the end of buffer
      while(t[++i] != '\n');
    }
    newT += t[i];
  }
  t = newT;
}

