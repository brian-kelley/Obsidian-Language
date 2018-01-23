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
  //generate generic map (hash table) implementation
  //this goes in utilFuncDecls/Defs
  void implHashTable();
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
  void generateStatement(ostream& c, Block* b, Statement* stmt);
  void generateBlock(ostream& c, Block* b);
  string generateExpression(ostream& c, Block* b, Expression* expr);
  void generateComparison(ostream& c, Oper op, string lhs, string rhs);
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
  string getDeallocFunc(TypeSystem::Type* t);
  string getPrintFunc(TypeSystem::Type* t);
  //convert and deep-copy input from one type to another
  //precondition: out->canConvert(in)
  string getConvertFunc(TypeSystem::Type* out, TypeSystem::Type* in);
  //compare two inputs for equality
  string getEqualsFunc(TypeSystem::Type* t);
  //test first < second
  string getLessFunc(TypeSystem::Type* t);
  //given two arrays, return a new concatenated array
  string getConcatFunc(TypeSystem::ArrayType* at);
  string getAppendFunc(TypeSystem::ArrayType* at);
  string getPrependFunc(TypeSystem::ArrayType* at);
  //generate "void sort_T_(T** data, size_type n)"
  string getSortFunc(TypeSystem::Type* t);
  //given an array and an index, "safely" access the element
  //program terminates if out of bounds
  string getAccessFunc(TypeSystem::ArrayType* at);
  string getAssignFunc(TypeSystem::ArrayType* at);
  string getHashFunc(TypeSystem::Type* t);
  string getHashInsert(TypeSystem::Type* t);

  //System for keeping track of and freeing objects in C scopes
  struct CVar
  {
    CVar() : type(nullptr), name("") {}
    CVar(TypeSystem::Type* t, string n) : type(t), name(n) {}
    TypeSystem::Type* type;
    string name;
  };
  struct CScope
  {
    vector<CVar> vars;
  };
  extern vector<CScope> cscopes;
  //create a new, empty scope
  void pushScope();
  //add a local variable to topmost scope
  void addScopedVar(TypeSystem::Type* t, string name);
  //generate free calls for all vars in top scope and then delete it
  void popScope(ostream& c);
}

#endif

