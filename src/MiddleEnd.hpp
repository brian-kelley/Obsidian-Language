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
  -If type doesn't exist for variable and can't be found, is a semantic error
  -Array/tuple types are created lazily, in special lists separate from scopes
*/

extern ModuleScope* global;

namespace MiddleEnd
{
  using namespace std;
  void load(AP(Parser::Module)& ast);
  //AST traversal functions for building scope tree
  namespace TypeLoading
  {
    //Types can only come from scoped decls
    void visitModule(Scope* current, AP(Parser::Module)& m);
    void visitBlock(Scope* current, AP(Parser::Block)& b);
    void visitStruct(Scope* current, AP(Parser::StructDecl)& sd);
    void visitScopedDecl(Scope* current, AP(Parser::ScopedDecl)& sd);
    void resolveAll();
  }
}

#endif

