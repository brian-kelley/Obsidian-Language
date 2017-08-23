#ifndef C_GEN_H
#define C_GEN_H

#include "Common.hpp"
#include "MiddleEnd.hpp"

namespace C
{
  //Generate C source, then run C compiler, and if !keep delete the source
  void generate(string outputStem, bool keep);
  //add file label and basic libc includes
  void genCommon();
  //forward-declare all compound types (and arrays),
  //then actually define them as C structs
  void genTypeDecls();
  //declare all global/static data
  void genGlobals();
  //forward-declare all subroutines, then actually provide impls
  void genSubroutines();

  //Utilities

  //Generate a unique C identifier (also won't collide with any existing C name)
  string getIdentifier();
  //given lambda f that takes a Scope*, run f on all scopes (depth-first)
  template<typename F> void walkScopeTree(F f);
  void generateCompoundType(ostream& c, string cName, TypeSystem::Type* t);
  void generateStatement(ostream& c, Block* b, Statement* stmt);
  void generateBlock(ostream& c, Block* b);
  void generateExpression(ostream& c, Block* b, Expression* expr);
  void generateAssignment(ostream& c, Block* b, Expression* lhs, Expression* rhs);
  //lazily generate (or return existing) print function for given compound type
  //for primitives, don't use this, just generate a single inline printf call
  string getPrintFunction(TypeSystem::Type* t);
  void generateCharLiteral(ostream& c, char character);
}

#endif

