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
  AP(Scope) loadScopes(AP(Parser::ModuleDef)& ast);
  void loadBuiltinTypes(AP(Scope)& global);
  void semanticCheck(AP(Scope)& global);
  void checkEntryPoint(AP(Scope)& global);
  //AST inorder traversal for building scope tree
  void visitModule(Scope* current, AP(Parser::Module)& module);
  void visitBlock(Scope* current, AP(Parser::Block)& module);
  void visitStruct(Scope* current, AP(Parser::StructDecl)& module);
}

#endif

