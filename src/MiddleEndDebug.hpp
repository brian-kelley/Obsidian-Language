#ifndef MIDDLE_END_DEBUG
#define MIDDLE_END_DEBUG

#include "MiddleEnd.hpp"

namespace MiddleEndDebug
{
  //main() should call this to print all info
  void printTypeTree();

  //General scope - calls one of the 3 specializations below
  void printScope(Scope* s, int ind);

  void printModuleScope(ModuleScope* s, int ind);
  void printStructScope(StructScope* s, int ind);
  void printBlockScope(BlockScope* s, int ind);

  //General type - calls one of the 6 specializations below
  void printType(Type* t, int ind);

  void printStructType(StructType* t, int ind);
  void printUnionType(UnionType* t, int ind);
  void printAliasType(AliasType* t, int ind);
  void printEnumType(EnumType* t, int ind);
  void printArrayType(ArrayType* t, int ind);
  void printTupleType(TupleType* t, int ind);
  void printFuncType(FuncPrototype* t, int ind);
  void printProcType(ProcPrototype* t, int ind);
}

#endif
