#include "Parser.hpp"
#include "Token.hpp"
#include <iostream>

void printAST(UP(ModuleDef)& ast);

namespace AstPrinter
{
  void setIndentLevel(int il);
  extern int indentLevel;

  void printModule(UP(Module)& m, int indent);
  void printScopedDecl(UP(ScopedDecl)& m, int indent);
  void printType(UP(Type)& t, int indent);
  void printStatement(UP(Statement)& s, int indent);
  void printTypedef(UP(Typedef)& t, int indent);
  void printReturn(UP(Return)& r, int indent);
  void printSwitch(UP(Switch)& s, int indent);
  void printContinue(UP(Continue)& c, int indent);
  void printBreak(UP(Break)& b, int indent);
  void printEmptyStatement(UP(Statement)& s, int indent);
  void printFor(UP(For)& f, int indent);
  void printWhile(UP(While)& w, int indent);
  void printIf(UP(If)& i, int indent);
  void printUsing(UP(Using)& u, int indent);
  void printAssertion(UP(Assertion)& a, int indent);
  void printTestDecl(UP(TestDecl)& td, int indent);
  void printEnum(UP(Enum)& e, int indent);
  void printBlock(UP(Block)& b, int indent);
  void printVarDecl(UP(VarDecl)& vd, int indent);
  void printVarAssign(UP(VarAssign)& va, int indent);
  void printPrint(UP(Print)& p, int indent);
  void printExpression(UP(Expression)& e, int indent);
  void printCall(UP(Call)& c, int indent);
  void printArg(UP(Arg)& a, int indent);
  void printFuncDecl(UP(FuncDecl)& fd, int indent);
  void printFuncDef(UP(FuncDef)& fd, int indent);
  void printFuncType(UP(FuncType)& ft, int indent);
  void printProcDecl(UP(ProcDecl)& pd, int indent);
  void printProcDef(UP(ProcDef)& pd, int indent);
  void printProcType(UP(ProcType)& pt, int indent);
  void printStructDecl(UP(StructDecl)& sd, int indent);
  void printVariantDecl(UP(VariantDecl)& vd, int indent);
  void printTraitDecl(UP(TraitDecl)& td, int indent);
  void printStructLit(UP(StructLit)& sl, int indent);
  void printMember(UP(Member)& m, int indent);
  void printTraitType(UP(TraitType)& tt, int indent);
  void printTupleType(UP(TupleType)& tt, int indent);
  void printBoolLit(UP(TupleType)& bl, int indent);
  void printExpr1(UP(Expr1)& e, int indent);
  void printExpr1RHS(UP(Expr1RHS)& e, int indent);
  void printExpr2(UP(Expr2)& e, int indent);
  void printExpr2RHS(UP(Expr2RHS)& e, int indent);
  void printExpr3(UP(Expr3)& e, int indent);
  void printExpr3RHS(UP(Expr3RHS)& e, int indent);
  void printExpr4(UP(Expr4)& e, int indent);
  void printExpr4RHS(UP(Expr4RHS)& e, int indent);
  void printExpr5(UP(Expr5)& e, int indent);
  void printExpr5RHS(UP(Expr5RHS)& e, int indent);
  void printExpr6(UP(Expr6)& e, int indent);
  void printExpr6RHS(UP(Expr6RHS)& e, int indent);
  void printExpr7(UP(Expr7)& e, int indent);
  void printExpr7RHS(UP(Expr7RHS)& e, int indent);
  void printExpr8(UP(Expr8)& e, int indent);
  void printExpr8RHS(UP(Expr8RHS)& e, int indent);
  void printExpr9(UP(Expr9)& e, int indent);
  void printExpr9RHS(UP(Expr9RHS)& e, int indent);
  void printExpr10(UP(Expr10)& e, int indent);
  void printExpr10RHS(UP(Expr10RHS)& e, int indent);
  void printExpr11(UP(Expr11)& e, int indent);
  void printExpr11RHS(UP(Expr11RHS)& e, int indent);
  void printExpr12(UP(Expr12)& e, int indent);
}

