#ifndef DEFERRED_LOOKUP
#define DEFERRED_LOOKUP

#include "Common.hpp"
#include "Parser.hpp"

/*
 * Deferred lookup is a generalized way to look up something that doesn't
 * need to be declared before use (types, traits, subroutines, modules, struct members)
 *
 * DeferredLookup knows how to perform the lookup function, and will remember the lookup
 * arguments to call during flush() in case the original lookup failed
 *
 * LookupFunc/ErrMsgFunc can be function ptr, lambda or functor
 *
 * LookupObject is probably a pointer to something (e.g. Type*)
 *
 * LookupFunc must have signature LookupObject* func(LookupArgs& args); //args doesn't have to be a ref
 * ErrMsgFunc must have signature string func(LookupArgs& args);        //""
 *
 * LookupArgs should be small and cheap to copy, because it will be stored persistently
 */

template<typename LookupObject, typename LookupFunc, typename LookupArgs, typename ErrMsgFunc>
struct DeferredLookup
{
  DeferredLookup(LookupFunc func, ErrMsgFunc errMsgFunc) : lookupFunc(func), errMsg(errMsgFunc) {}
  void lookup(LookupArgs& args, LookupObject*& object)
  {
    LookupObject* found = lookupFunc(args);
    if(!found)
    {
      deferredArgs.push_back(args);
      deferredUsage.push_back(&object);
    }
    else
    {
      object = found;
    }
  }
  void flush()
  {
    for(size_t i = 0; i < deferredArgs.size(); i++)
    {
      LookupObject* found = lookupFunc(deferredArgs[i]);
      if(found == nullptr)
      {
        //lookup failed on second pass, is an error
        ERR_MSG(errMsg(deferredArgs[i]));
      }
      *(deferredUsage[i]) = found;
    }
    deferredArgs.clear();
    deferredUsage.clear();
  }
  vector<LookupArgs> deferredArgs;
  vector<LookupObject**> deferredUsage;
  LookupFunc lookupFunc;
  ErrMsgFunc errMsg;
};

#endif

