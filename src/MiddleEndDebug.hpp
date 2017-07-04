#ifndef MIDDLE_END_DEBUG
#define MIDDLE_END_DEBUG

#include "MiddleEnd.hpp"

namespace MiddleEndDebug
{
  //main() should call this to print all info
  void printTypeTree();

  //General scope - calls one of the 3 specializations below
  void printScope(Scope* s, int ind);
  void printScopeBody(Scope* s, int ind);

  //General type - calls one of the 6 specializations below
  void printType(TypeSystem::Type* t, int ind);

  void printStructType(TypeSystem::StructType* t, int ind);
  void printUnionType(TypeSystem::UnionType* t, int ind);
  void printAliasType(TypeSystem::AliasType* t, int ind);
  void printEnumType(TypeSystem::EnumType* t, int ind);
  void printArrayType(TypeSystem::ArrayType* t, int ind);
  void printTupleType(TypeSystem::TupleType* t, int ind);
  void printFuncType(TypeSystem::FuncType* t, int ind);
  void printProcType(TypeSystem::ProcType* t, int ind);
  
  void printTrait(TypeSystem::Trait* t, int ind);
}

#endif
