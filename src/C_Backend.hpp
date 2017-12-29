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
  void genMain(Subroutine* m);

  //Generate a unique C identifier (also won't collide with any existing C name)
  string getIdentifier();
  //given lambda f that takes a Scope*, run f on all scopes (depth-first)
  template<typename F> void walkScopeTree(F f);
  void generateCompoundType(ostream& c, string cName, TypeSystem::Type* t);
  void generateStatement(ostream& c, Block* b, Statement* stmt);
  void generateBlock(ostream& c, Block* b);
  void generateExpression(ostream& c, Expression* expr);
  void generateAssignment(ostream& c, Block* b, Expression* lhs, Expression* rhs);
  void generateLocalVariables(ostream& c, BlockScope* b);
  //utility functions
  //generate a nicely formatted "section" header in C comment
  void generateSectionHeader(ostream& c, string name);
  //lazily generate and return name of C function
  //(also generates all other necessary util funcs)
  string getInitFunc(TypeSystem::Type* t);
  string getCopyFunc(TypeSystem::Type* t);
  //alloc functions take N integers and produce N-dimensional rectangular array
  string getAllocFunc(TypeSystem::ArrayType* t);
  //precondition: typeNeedsDealloc(t)
  string getDeallocFunc(TypeSystem::Type* t);
  string getPrintFunc(TypeSystem::Type* t);
  //convert and deep-copy input from one type to another
  //precondition: out->canConvert(in)
  string getConvertFunc(TypeSystem::Type* out, TypeSystem::Type* in);
  //compare two inputs for equality
  string getEqualsFunc(TypeSystem::Type* t);
  //test first < second
  string getLessFunc(TypeSystem::Type* t);
  //test first <= second
  string getLessEqFunc(TypeSystem::Type* t);
  //whether a type may own heap-allocated memory
  bool typeNeedsDealloc(TypeSystem::Type* t);
}

#endif

