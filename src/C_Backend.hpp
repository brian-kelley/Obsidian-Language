#ifndef C_GEN_H
#define C_GEN_H

#include "Common.hpp"
#include "TypeSystem.hpp"
#include "Subroutine.hpp"
#include "Expression.hpp"
#include "Variable.hpp"
#include "IR.hpp"

/* C backend memory management:
 *
 *  -global variables are statically allocated
 *  -local variables are stack allocated
 *  -array, map and union storage are heap allocated
 *  -primitives/enums passed by value; everything else by pointer
 */

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

  //Generate a unique integer to be used as C identifier
  string getIdentifier();

  //generate a nicely formatted "section" header in a comment
  void generateSectionHeader(ostream& c, string name);

  void emitStatement(ostream& c, IR::StatementIR* stmt);

  //utility functions
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
  //true if type needs "->" to access members instead of "."
  bool isPointer(Expression* expr);

  //Generate free calls for all variables inside of current,
  //and all its parent scopes up to and including s.
  //Call before generating return, end of block, etc.
  //This is like automatic generation of C++ destructors
  void freeWithinScope(ostream& c, Scope* s, Scope* current);
}

#endif

