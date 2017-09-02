#ifndef C_GEN_H
#define C_GEN_H

#include "Common.hpp"
#include "MiddleEnd.hpp"

namespace C
{
  //Generate C source file to outputStem.c, then run C compiler, and if !keep delete the source file
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
  //generate the code to print expr (may use getPrintFunction)
  void generatePrint(ostream& c, Block* b, Expression* expr);
  void generateCharLiteral(ostream& c, char character);
  //generate function to allocate an array (taking one uint64 for each dimension)
  void generateNewArrayFunction(ostream& c, string ident, TypeSystem::ArrayType* at);
}

#endif

