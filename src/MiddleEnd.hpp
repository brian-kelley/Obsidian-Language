#ifndef MIDDLE_END_H
#define MIDDLE_END_H

#include "Common.hpp"
#include "Parser.hpp"
#include "Scope.hpp"
#include "TypeSystem.hpp"
#include "Subroutine.hpp"
#include "Variable.hpp"
#include "Expression.hpp"

/*
Middle-end processing has several steps:
-build the scope tree (modules, structs, traits, subroutines, blocks)
  -all types should be available from either in same scope or an above scope
  -If type doesn't exist for variable and can't be found, is a semantic error
  -Anonymous (array, tuple, union, map) types are created lazily, in special lists separate from scopes
*/

extern ModuleScope* global;

namespace MiddleEnd
{
  void load(Parser::Module* ast);
  //parse tree traversal functions for building scope tree and loading all type decls
  namespace ScopeTypeLoading
  {
    //Types can only come from scoped decls
    void visitModule(Scope* current, Parser::Module* m);
    void visitBlock(Scope* current, Parser::Block* b);
    void visitStatement(Scope* current, Parser::StatementNT* s);
    void visitStruct(Scope* current, Parser::StructDecl* sd);
    void visitFuncDef(Scope* current, Parser::FuncDef* fd);
    void visitProcDef(Scope* current, Parser::ProcDef* pd);
    void visitTrait(Scope* current, Parser::TraitDecl* td);
    void visitScopedDecl(Scope* current, Parser::ScopedDecl* sd);
    //for loops are special becuase they always introduce a block scope (for the counter),
    //even if its body is not a block
    void resolveAll();
  }
  //adding variables to scopes
  namespace VarLoading
  {
    void visitScope(Scope* s);
    void visitVarDecl(Scope* s, Parser::VarDecl* vd);
  }
  namespace SubroutineLoading
  {
    void visitScope(Scope* s);
    void visitDecl(Scope* s, Parser::ScopedDecl* decl);
  }
}

#endif

