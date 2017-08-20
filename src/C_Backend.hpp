#ifndef C_GEN_H
#define C_GEN_H

#include "Utils.hpp"
#include "MiddleEnd.hpp"

namespace C
{
  //Generate C source, then run C compiler, and if !keep delete the source
  void generate(string outputStem, bool keep);
  //add file label and basic libc includes
  void genCommon(ostream& c);
  //forward-declare all compound types (and arrays),
  //then actually define them as C structs
  void genTypeDecls(ostream& c);
  //declare all global/static data
  void genGlobals(ostream& c);
  //forward-declare all subroutines, then actually provide impls
  void genSubroutines(ostream& c);

  //Utilities
  //given lambda f that takes a Scope*, run f on all scopes
  string getIdentifier();
  template<typename F>
  void walkScopeTree(F f);
  void generateCompoundType(ostream& c, string cName, TypeSystem::Type* t);
}

#endif

