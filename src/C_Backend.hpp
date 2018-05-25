#ifndef C_GEN_H
#define C_GEN_H

#include "Common.hpp"
#include "TypeSystem.hpp"
#include "Subroutine.hpp"
#include "Expression.hpp"
#include "Variable.hpp"

namespace C
{
  //Generate C source file to outputStem.c, then run C compiler, and if !keep delete the source file
  void generate(string outputStem, bool keep);
  //add file label and basic libc includes
  void genCommon();
  //generate generic map (hash table) implementation
  //this goes in utilFuncDecls/Defs
  void implHashTable();
  //generate these core builtin subroutines that can't be written in Onyx
  //(necessary for real compiler):
  //  proc ubyte[] readFile(char[] filename)
  //  proc void writeFile(ubyte[] data, char[] filename)
  //  proc char[] readLine()
  //  proc char[] readToken()
  //  func uint floatRepr(float f)
  //  func ulong doubleRepr(double d)
  //
  //  Note: following builtins are implemented in Onyx (see BuiltIn.cpp)
  //
  //  func long? stoi(char[] str)
  //  func double? stod(char[] str)
  //  func char[] printHex(ulong num)
  //  func char[] printBin(ulong num)
  void genCoreBuiltins();
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
  void generateComparison(ostream& c, int op, Type* t, string lhs, string rhs);
  void generateAssignment(ostream& c, Block* b, Expression* lhs, Expression* rhs);
  void generateLocalVariables(ostream& c, Block* b);
  //utility functions
  //generate a nicely formatted "section" header in C comment
  void generateSectionHeader(ostream& c, string name);
  //lazily generate and return name of C function
  //(also generates all other necessary util funcs)
  string getInitFunc(Type* t);
  string getCopyFunc(Type* t);
  //alloc functions take N integers and produce N-dimensional rectangular array
  string getAllocFunc(ArrayType* t);
  string getDeallocFunc(Type* t);
  string getPrintFunc(Type* t);
  //convert and deep-copy input from one type to another
  //precondition: out->canConvert(in)
  string getConvertFunc(Type* out, Type* in);
  //compare two inputs for equality
  string getEqualsFunc(Type* t);
  //test first < second
  string getLessFunc(Type* t);
  //given two arrays, return a new concatenated array
  string getConcatFunc(ArrayType* at);
  string getAppendFunc(ArrayType* at);
  string getPrependFunc(ArrayType* at);
  //generate "void sort_T_(T** data, size_type n)"
  string getSortFunc(Type* t);
  //given an array and an index, "safely" access the element
  //program terminates if out of bounds
  string getAccessFunc(ArrayType* at);
  string getAssignFunc(ArrayType* at);
  string getHashFunc(Type* t);
  string getHashInsert(Type* t);

  //true if type owns heap memory
  bool typeNeedsDealloc(Type* t);
  //true if type is stack-allocated and passed by value
  bool isPOD(Type* t);

  //System for keeping track of and freeing objects in C scopes
  struct CVar
  {
    CVar() : type(nullptr), name("") {}
    CVar(Type* t, string n) : type(t), name(n) {}
    Type* type;
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
  void addScopedVar(Type* t, string name);
  //free all heap variables in top scope and then pop
  void popScope(ostream& c);
}

#endif

