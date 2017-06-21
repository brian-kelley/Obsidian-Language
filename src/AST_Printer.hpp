#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "Parser.hpp"
#include "Token.hpp"
#include <iostream>

void printAST(Parser::Module* ast);

namespace AstPrinter
{
  using namespace Parser;

  void setIndentLevel(int il);
  extern int indentLevel;

  void printModule(Module* m, int indent);
  void printScopedDecl(ScopedDecl* m, int indent);
  void printTypeNT(Parser::TypeNT* t, int indent);
  void printStatement(Statement* s, int indent);
  void printTypedef(Typedef* t, int indent);
  void printReturn(Return* r, int indent);
  void printSwitch(Switch* s, int indent);
  void printContinue(int indent);
  void printBreak(int indent);
  void printEmptyStatement(int indent);
  void printFor(For* f, int indent);
  void printWhile(While* w, int indent);
  void printIf(If* i, int indent);
  void printAssertion(Assertion* a, int indent);
  void printTestDecl(TestDecl* td, int indent);
  void printEnum(Enum* e, int indent);
  void printBlock(Block* b, int indent);
  void printVarDecl(VarDecl* vd, int indent);
  void printVarAssign(VarAssign* va, int indent);
  void printPrint(Print* p, int indent);
  void printExpression(Expression* e, int indent);
  void printCall(Call* c, int indent);
  void printArg(Arg* a, int indent);
  void printFuncDecl(FuncDecl* fd, int indent);
  void printFuncDef(FuncDef* fd, int indent);
  void printFuncType(FuncType* ft, int indent);
  void printProcDecl(ProcDecl* pd, int indent);
  void printProcDef(ProcDef* pd, int indent);
  void printProcType(ProcType* pt, int indent);
  void printStructDecl(StructDecl* sd, int indent);
  void printUnionDecl(UnionDecl* vd, int indent);
  void printTraitDecl(TraitDecl* td, int indent);
  void printStructLit(StructLit* sl, int indent);
  void printMember(Member* m, int indent);
  void printTraitType(TraitType* tt, int indent);
  void printTupleTypeNT(TupleTypeNT* tt, int indent);
  void printBoolLit(BoolLit* bl, int indent);
  void printExpr1(Expr1* e, int indent);
  void printExpr1RHS(Expr1RHS* e, int indent);
  void printExpr2(Expr2* e, int indent);
  void printExpr2RHS(Expr2RHS* e, int indent);
  void printExpr3(Expr3* e, int indent);
  void printExpr3RHS(Expr3RHS* e, int indent);
  void printExpr4(Expr4* e, int indent);
  void printExpr4RHS(Expr4RHS* e, int indent);
  void printExpr5(Expr5* e, int indent);
  void printExpr5RHS(Expr5RHS* e, int indent);
  void printExpr6(Expr6* e, int indent);
  void printExpr6RHS(Expr6RHS* e, int indent);
  void printExpr7(Expr7* e, int indent);
  void printExpr7RHS(Expr7RHS* e, int indent);
  void printExpr8(Expr8* e, int indent);
  void printExpr8RHS(Expr8RHS* e, int indent);
  void printExpr9(Expr9* e, int indent);
  void printExpr9RHS(Expr9RHS* e, int indent);
  void printExpr10(Expr10* e, int indent);
  void printExpr10RHS(Expr10RHS* e, int indent);
  void printExpr11(Expr11* e, int indent);
  void printExpr12(Expr12* e, int indent);
}

#endif

