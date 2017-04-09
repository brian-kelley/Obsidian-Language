#ifndef MIDDLE_END_H
#define MIDDLE_END_H

#include "Misc.hpp"
#include "Utils.hpp"
#include "Variable.hpp"
#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "Scope.hpp"

/*
Middle-end processing has several steps:
-build scope tree (module, block, struct)
  -tree is built depth-first
    -to visit a scope, load all types and
     traits declared there, then descend to child scopes
    -struct traits and members and variant types should be fully available
  -all types should be available from either in same scope or an above scope
  -If type doesn't exist for variable, is a semantic error
  -Array/tuple types are created lazily, in special lists separate from scopes
*/

namespace MiddleEnd
{
  using namespace std;
  using namespace Parser;
  void load(AP(Module)& ast);
  void loadBuiltinTypes(AP(Scope)& global);
  void semanticCheck(AP(Scope)& global);
  void checkEntryPoint(AP(Scope)& global);
  //AST traversal functions for building scope tree
  namespace TypeLoading
  {
    //Types can only come from scoped decls
    void visitModule(Scope* current, AP(Module)& m);
    void visitBlock(Scope* current, AP(Block)& b);
    void visitStruct(Scope* current, AP(StructDecl)& sd);
    void visitScopedDecl(Scope* current, AP(ScopedDecl)& sd);
  }
}

#endif

