#ifndef MIDDLE_END_H
#define MIDDLE_END_H

#include "Misc.hpp"
#include "Utils.hpp"
#include "Variable.hpp"
#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "Scope.hpp"

namespace MiddleEnd
{
  using namespace std;
  using namespace Parser;
  AP(Scope) loadScopes(AP(Module)& ast);
  void loadBuiltinTypes(AP(Scope)& global);
  void semanticCheck(AP(Scope)& global);
  void checkEntryPoint(AP(Scope)& global);
  //AST traversal functions for building scope tree
  //Note: Module, Block, Struct are all the scope types
  namespace TypeLoading
  {
    //Types can only come from scoped decls
    void visitModule(Scope* current, AP(Module)& module);
    void visitBlock(Scope* current, AP(Block)& module);
    void visitStruct(Scope* current, AP(StructDecl)& module);
  }
}

#endif

