#include "PreTokenize.hpp"

vector<string> PreTokenize::getTokens(Text t)
{
  //match " with ", and collapse escaped characters inside
  //match /* with */ (allow nesting), then delete text inside
  //match // with \n, then delete text inside
  for(size_t i = 0; i < t.size; i++)
  {
    if(t.buf[i] == '\"')
    {
    }
  }
}

