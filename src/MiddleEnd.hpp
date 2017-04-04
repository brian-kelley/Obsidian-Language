#ifndef MIDDLE_END_H
#define MIDDLE_END_H

#include "Misc.hpp"
#include "Utils.hpp"
#include "Variable.hpp"
#include "Parser.hpp"

namespace MiddleEnd
{
  void semanticCheck(AP(Parser::ModuleDef)& ast);
  void checkEntryPoint(AP(Parser::ModuleDef)& ast);
/*
  enum DeclType
  {
    VAR,
  };
  
  struct Scope
  {
    //NULL parent: global scope
    Scope* parent;
    static Scope* global;
    static int mangleNum;
  };

  struct Decl
  {
    Scope* enclosing;
  };

  struct Type
  {
  };

  struct PrimitiveType
  {
  };

  struct StructType
  {
  };

  struct ArrayType
  {
  };

  struct TupleType
  {
  };
*/
}

#endif

