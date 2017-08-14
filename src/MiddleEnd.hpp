#ifndef MIDDLE_END_H
#define MIDDLE_END_H

#include "Misc.hpp"
#include "Utils.hpp"
#include "Parser.hpp"
#include "Scope.hpp"
#include "TypeSystem.hpp"
#include "Subroutine.hpp"
#include "Variable.hpp"
#include "Expression.hpp"

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
  void load(Parser::Module* ast);
  //AST traversal functions for building scope tree and loading all type decls
  namespace ScopeTypeLoading
  {
    //Types can only come from scoped decls
    void visitModule(Scope* current, Parser::Module* m);
    void visitBlock(Scope* current, Parser::Block* b);
    void visitStatement(Scope* current, Parser::StatementNT* s);
    void visitStruct(Scope* current, Parser::StructDecl* sd);
    void visitScopedDecl(Scope* current, Parser::ScopedDecl* sd);
    //for loops are special becuase they always introduce a block scope (for the counter),
    //even if its body is not a block
    void resolveAll();
  }
  namespace SubroutineLoading
  {
    void visitScope(Scope* s);
    void visitFuncDef(Scope* s, Parser::FuncDef* ast);
    void visitProcDef(Scope* s, Parser::ProcDef* ast);
  }
  namespace VarLoading
  {
    void visitScope(Scope* s);
    void visitVarDecl(Scope* s, Parser::VarDecl* vd);
  }
}

#endif

