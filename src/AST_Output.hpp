#ifndef PARSE_TREE_OUTPUT_H
#define PARSE_TREE_OUTPUT_H

#include "Common.hpp"

struct Module;
struct Statement;
struct Expression;
struct Name;
struct Type;
struct StructType;
struct AliasType;
struct Subroutine;
struct ExternalSubroutine;
struct SimpleType;
struct EnumType;
struct Variable;

void outputAST(Module* tree, string filename);

namespace AstOut
{
  int emitModule(Module* m);
  int emitName(Name* n);
  int emitType(Type* t);
  int emitStatement(Statement* s);
  int emitExpression(Expression* e);
  int emitStruct(StructType* s);
  int emitAlias(AliasType* a);
  int emitSubroutine(Subroutine* s);
  int emitExternSubroutine(ExternalSubroutine* s);
  int emitVariable(Variable* v);
  int emitEnum(EnumType* e);
  int emitSimpleType(SimpleType* s);
}

#endif

