#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "Parser.hpp"
#include "Token.hpp"
#include <iostream>

void printAST(AP(Parser::Module)& ast);

namespace AstPrinter
{
  using namespace Parser;

  void setIndentLevel(int il);
  extern int indentLevel;

  void printModule(AP(Module)& m, int indent);
  void printScopedDecl(AP(ScopedDecl)& m, int indent);
  void printTypeNT(AP(Parser::TypeNT)& t, int indent);
  void printStatement(AP(Statement)& s, int indent);
  void printTypedef(AP(Typedef)& t, int indent);
  void printReturn(AP(Return)& r, int indent);
  void printSwitch(AP(Switch)& s, int indent);
  void printContinue(int indent);
  void printBreak(int indent);
  void printEmptyStatement(int indent);
  void printFor(AP(For)& f, int indent);
  void printWhile(AP(While)& w, int indent);
  void printIf(AP(If)& i, int indent);
  void printAssertion(AP(Assertion)& a, int indent);
  void printTestDecl(AP(TestDecl)& td, int indent);
  void printEnum(AP(Enum)& e, int indent);
  void printBlock(AP(Block)& b, int indent);
  void printVarDecl(AP(VarDecl)& vd, int indent);
  void printVarAssign(AP(VarAssign)& va, int indent);
  void printPrint(AP(Print)& p, int indent);
  void printExpression(AP(Expression)& e, int indent);
  void printCall(AP(Call)& c, int indent);
  void printArg(AP(Arg)& a, int indent);
  void printFuncDecl(AP(FuncDecl)& fd, int indent);
  void printFuncDef(AP(FuncDef)& fd, int indent);
  void printFuncType(AP(FuncType)& ft, int indent);
  void printProcDecl(AP(ProcDecl)& pd, int indent);
  void printProcDef(AP(ProcDef)& pd, int indent);
  void printProcType(AP(ProcType)& pt, int indent);
  void printStructDecl(AP(StructDecl)& sd, int indent);
  void printUnionDecl(AP(UnionDecl)& vd, int indent);
  void printTraitDecl(AP(TraitDecl)& td, int indent);
  void printStructLit(AP(StructLit)& sl, int indent);
  void printMember(AP(Member)& m, int indent);
  void printTraitType(AP(TraitType)& tt, int indent);
  void printTupleTypeNT(AP(TupleTypeNT)& tt, int indent);
  void printBoolLit(AP(BoolLit)& bl, int indent);
  void printExpr1(AP(Expr1)& e, int indent);
  void printExpr1RHS(AP(Expr1RHS)& e, int indent);
  void printExpr2(AP(Expr2)& e, int indent);
  void printExpr2RHS(AP(Expr2RHS)& e, int indent);
  void printExpr3(AP(Expr3)& e, int indent);
  void printExpr3RHS(AP(Expr3RHS)& e, int indent);
  void printExpr4(AP(Expr4)& e, int indent);
  void printExpr4RHS(AP(Expr4RHS)& e, int indent);
  void printExpr5(AP(Expr5)& e, int indent);
  void printExpr5RHS(AP(Expr5RHS)& e, int indent);
  void printExpr6(AP(Expr6)& e, int indent);
  void printExpr6RHS(AP(Expr6RHS)& e, int indent);
  void printExpr7(AP(Expr7)& e, int indent);
  void printExpr7RHS(AP(Expr7RHS)& e, int indent);
  void printExpr8(AP(Expr8)& e, int indent);
  void printExpr8RHS(AP(Expr8RHS)& e, int indent);
  void printExpr9(AP(Expr9)& e, int indent);
  void printExpr9RHS(AP(Expr9RHS)& e, int indent);
  void printExpr10(AP(Expr10)& e, int indent);
  void printExpr10RHS(AP(Expr10RHS)& e, int indent);
  void printExpr11(AP(Expr11)& e, int indent);
  void printExpr12(AP(Expr12)& e, int indent);
}

#endif

