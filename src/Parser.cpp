#include "Parser.hpp"

Program parse(vector<Token*> toks)
{
  Program p;
  //TODO: global typedefs ahead of functions
  //For now, only do functions
  //Function pattern: ident ident ( a b, c d, ... ) { ... }
  TokIter it = toks.begin();
  while(true)
  {
    TokIter fstart = it;
    //check that first two args are identifiers
    Ident* id = dynamic_cast<Ident>(*it);
    if(!id)
    {
      errAndExit("Function has missing return type.");
    }
    it++;
    Ident* id = dynamic_cast<Ident>(*it);
    if(!id)
    {
      errAndExit("Function has invalid or missing name.");
    }
    it++;
    //seek past arg list to next { token
    while(true)
    {
      Punct* punct = dynamic_cast<Punct>(*it);
      if(punct && punct->val == LBRACE)
        break;
      it++;
    }
    it++;
    while(true)
    {
      Punct* punct = dynamic_cast<Punct>(*it);
      if(punct && punct->val == RBRACE)
        break;
      it++;
    }
    //it is the last token in the function
    p.funcs.push_back(parseFunc(fstart, it));
    it++;
    if(it == toks.end())
      break;
  }
  return p;
}

