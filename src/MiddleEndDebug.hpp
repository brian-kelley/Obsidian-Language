#ifndef MIDDLE_END_DEBUG
#define MIDDLE_END_DEBUG

#include "MiddleEnd.hpp"

namespace MiddleEndDebug
{
  //main() should call this to print all info
  void printTypeTree();

  //General scope - calls one of the 3 specializations below
  void printScope(Scope* s, int indent);

  void printModuleScope(ModuleScope* s, int indent);
  void printStructScope(StructScope* s, int indent);
  void printBlockScope(BlockScope* s, int indent);

  //General type - calls one of the 6 specializations below
  void printType(Type* t, int indent);

  void printStructType(StructType* t, int indent);
  void printUnionType(UnionType* t, int indent);
  void printAliasType(AliasType* t, int indent);
  void printEnumType(EnumType* t, int indent);
  void printArrayType(ArrayType* t, int indent);
  void printTupleType(TupleType* t, int indent);
}

#endif
