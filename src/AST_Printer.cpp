#include "AST_Printer.hpp"

using namespace std;

void printAST(AP(Parser::Module)& ast)
{
  try
  {
    AstPrinter::printModule(ast, -2);
  }
  catch(exception& e)
  {
    cout << "Error while printing AST: " << e.what() << '\n';
  }
}

namespace AstPrinter
{
  using namespace Parser;

  //spaces of indentation
  int indentLevel = 2;
  void indent(int il)
  {
    for(int i = 0; i < il; i++)
    {
      cout << ' ';
    }
  }

  void printModule(AP(Module)& m, int ind)
  {
    indent(ind);
    if(m->name.length())
    {
      cout << "Module \"" << m->name << "\"\n";
    }
    for(auto& it : m->decls)
    {
      printScopedDecl(it, ind + indentLevel);
    }
  }


  void printScopedDecl(AP(ScopedDecl)& m, int ind)
  {
    indent(ind);
    cout << "ScopedDecl\n";
    if(m->decl.is<AP(Module)>())
      printModule(m->decl.get<AP(Module)>(), ind + indentLevel);
    else if(m->decl.is<AP(VarDecl)>())
      printVarDecl(m->decl.get<AP(VarDecl)>(), ind + indentLevel);
    else if(m->decl.is<AP(StructDecl)>())
      printStructDecl(m->decl.get<AP(StructDecl)>(), ind + indentLevel);
    else if(m->decl.is<AP(UnionDecl)>())
      printUnionDecl(m->decl.get<AP(UnionDecl)>(), ind + indentLevel);
    else if(m->decl.is<AP(TraitDecl)>())
      printTraitDecl(m->decl.get<AP(TraitDecl)>(), ind + indentLevel);
    else if(m->decl.is<AP(Enum)>())
      printEnum(m->decl.get<AP(Enum)>(), ind + indentLevel);
    else if(m->decl.is<AP(Typedef)>())
      printTypedef(m->decl.get<AP(Typedef)>(), ind + indentLevel);
    else if(m->decl.is<AP(FuncDecl)>())
      printFuncDecl(m->decl.get<AP(FuncDecl)>(), ind + indentLevel);
    else if(m->decl.is<AP(FuncDef)>())
      printFuncDef(m->decl.get<AP(FuncDef)>(), ind + indentLevel);
    else if(m->decl.is<AP(ProcDecl)>())
      printProcDecl(m->decl.get<AP(ProcDecl)>(), ind + indentLevel);
    else if(m->decl.is<AP(ProcDef)>())
      printProcDef(m->decl.get<AP(ProcDef)>(), ind + indentLevel);
    else if(m->decl.is<AP(TestDecl)>())
      printTestDecl(m->decl.get<AP(TestDecl)>(), ind + indentLevel);
  }

  void printTypeNT(AP(Parser::TypeNT)& t, int ind)
  {
    indent(ind);
    cout << "Type: ";
    if(t->arrayDims)
    {
      cout << t->arrayDims << "-dim array of ";
    }
    if(t->t.is<TypeNT::Prim>())
    {
      //primitive
      TypeNT::Prim p = t->t.get<TypeNT::Prim>();
      cout << "primitive ";
      switch(p)
      {
        case TypeNT::Prim::BOOL:
          cout << "bool";
          break;
        case TypeNT::Prim::CHAR:
          cout << "char";
          break;
        case TypeNT::Prim::UCHAR:
          cout << "uchar";
          break;
        case TypeNT::Prim::SHORT:
          cout << "short";
          break;
        case TypeNT::Prim::USHORT:
          cout << "ushort";
          break;
        case TypeNT::Prim::INT:
          cout << "int";
          break;
        case TypeNT::Prim::UINT:
          cout << "uint";
          break;
        case TypeNT::Prim::LONG:
          cout << "long";
          break;
        case TypeNT::Prim::ULONG:
          cout << "ulong";
          break;
        case TypeNT::Prim::FLOAT:
          cout << "float";
          break;
        case TypeNT::Prim::DOUBLE:
          cout << "double";
          break;
        case TypeNT::Prim::STRING:
          cout << "string";
          break;
        default:
          cout << "invalid primitive";
      }
      cout << '\n';
    }
    else if(t->t.is<AP(Member)>())
    {
      //member, print indented on next line
      cout << '\n';
      printMember(t->t.get<AP(Member)>(), ind + indentLevel);
    }
    else if(t->t.is<AP(TupleTypeNT)>())
    {
      //tuple type, print indented on next line
      cout << '\n';
      printTupleTypeNT(t->t.get<AP(TupleTypeNT)>(), ind + indentLevel);
    }
  }

  void printStatement(AP(Statement)& s, int ind)
  {
    //statements don't need any extra printouts
    if(s->s.is<AP(ScopedDecl)>())
      printScopedDecl(s->s.get<AP(ScopedDecl)>(), ind);
    else if(s->s.is<AP(VarAssign)>())
      printVarAssign(s->s.get<AP(VarAssign)>(), ind);
    else if(s->s.is<AP(Print)>())
      printPrint(s->s.get<AP(Print)>(), ind);
    else if(s->s.is<AP(Expression)>())
      printExpression(s->s.get<AP(Expression)>(), ind);
    else if(s->s.is<AP(Block)>())
      printBlock(s->s.get<AP(Block)>(), ind);
    else if(s->s.is<AP(Return)>())
      printReturn(s->s.get<AP(Return)>(), ind);
    else if(s->s.is<AP(Continue)>())
      printContinue(ind);
    else if(s->s.is<AP(Break)>())
      printBreak(ind);
    else if(s->s.is<AP(Switch)>())
      printSwitch(s->s.get<AP(Switch)>(), ind);
    else if(s->s.is<AP(For)>())
      printFor(s->s.get<AP(For)>(), ind);
    else if(s->s.is<AP(While)>())
      printWhile(s->s.get<AP(While)>(), ind);
    else if(s->s.is<AP(If)>())
      printIf(s->s.get<AP(If)>(), ind);
    else if(s->s.is<AP(Assertion)>())
      printAssertion(s->s.get<AP(Assertion)>(), ind);
    else if(s->s.is<AP(EmptyStatement)>())
      printEmptyStatement(ind);
    else if(s->s.is<AP(VarDecl)>())
      printVarDecl(s->s.get<AP(VarDecl)>(), ind);
  }

  void printTypedef(AP(Typedef)& t, int ind)
  {
    indent(ind);
    cout << "Typedef \"" << t->ident << "\"\n";
    printTypeNT(t->type, ind + indentLevel);
  }

  void printReturn(AP(Return)& r, int ind)
  {
    indent(ind);
    cout << "Return\n";
    if(r->ex)
      printExpression(r->ex, ind + 2);
  }

  void printSwitch(AP(Switch)& s, int ind)
  {
    indent(ind);
    cout << "Switch\n";
    indent(ind + indentLevel);
    cout << "Value\n";
    printExpression(s->sw, ind + indentLevel);
    for(auto sc : s->cases)
    {
      indent(ind + indentLevel);
      cout << "Match value:\n";
      printExpression(sc->matchVal, ind + indentLevel);
      indent(ind + indentLevel);
      cout << "Match statement:\n";
      printStatement(sc->s, ind + indentLevel);
    }
    if(s->defaultStatement)
    {
      indent(ind + indentLevel);
      cout << "Default statement:\n";
      printStatement(s->defaultStatement, ind + indentLevel);
    }
  }

  void printContinue(int ind)
  {
    indent(ind);
    cout << "Continue\n";
  }

  void printBreak(int ind)
  {
    indent(ind);
    cout << "Break\n";
  }

  void printEmptyStatement(int ind)
  {
    indent(ind);
    cout << "Empty Statement\n";
  }

  void printFor(AP(For)& f, int ind)
  {
    indent(ind);
    cout << "For ";
    if(f->f.is<AP(ForC)>())
    {
      cout << "C-style\n";
      auto& forC = f->f.get<AP(ForC)>();
      indent(ind + indentLevel);
      cout << "Initializer: ";
      if(forC->decl)
      {
        cout << '\n';
        printVarDecl(forC->decl, ind + indentLevel);
      }
      else
      {
        cout << "none\n";
      }
      indent(ind + indentLevel);
      cout << "Condition: ";
      if(forC->condition)
      {
        cout << '\n';
        printExpression(forC->condition, ind + indentLevel);
      }
      else
      {
        cout << "none\n";
      }
      indent(ind + indentLevel);
      cout << "Increment: ";
      if(forC->incr)
      {
        cout << '\n';
        printVarAssign(forC->incr, ind + indentLevel);
      }
      else
      {
        cout << "none\n";
      }
    }
    else if(f->f.is<AP(ForRange1)>())
    {
      cout << "[0, n) range\n";
      auto& forRange1 = f->f.get<AP(ForRange1)>();
      indent(ind + indentLevel);
      cout << "Upper bound:\n";
      printExpression(forRange1->expr, ind + indentLevel);
    }
    else if(f->f.is<AP(ForRange2)>())
    {
      cout << "[n1, n2) range\n";
      auto& forRange2 = f->f.get<AP(ForRange2)>();
      indent(ind + indentLevel);
      cout << "Lower bound:\n";
      printExpression(forRange2->start, ind + indentLevel);
      cout << "Upper bound:\n";
      printExpression(forRange2->end, ind + indentLevel);
    }
    else if(f->f.is<AP(ForArray)>())
    {
      cout << "container\n";
      auto& forArray = f->f.get<AP(ForArray)>();
      indent(ind + indentLevel);
      cout << "Container:\n";
      printExpression(forArray->container, ind + indentLevel);
    }
    indent(ind);
    cout << "Body:\n";
    printStatement(f->body, ind + indentLevel);
  }

  void printWhile(AP(While)& w, int ind)
  {
    indent(ind);
    cout << "While\n";
    indent(ind + indentLevel);
    cout << "Condition:\n";
    printExpression(w->cond, ind + indentLevel);
    indent(ind + indentLevel);
    cout << "Body:\n";
    printStatement(w->body, ind + indentLevel);
  }

  void printIf(AP(If)& i, int ind)
  {
    indent(ind);
    cout << "If\n";
    indent(ind + indentLevel);
    cout << "Condition\n";
    printExpression(i->cond, ind + indentLevel);
    indent(ind + indentLevel);
    cout << "If Body\n";
    printStatement(i->ifBody, ind + indentLevel);
    if(i->elseBody)
    {
      indent(ind + indentLevel);
      cout << "Else Body\n";
      printStatement(i->elseBody, ind + indentLevel);
    }
  }

  void printAssertion(AP(Assertion)& a, int ind)
  {
    indent(ind);
    cout << "Assertion\n";
    printExpression(a->expr, ind + indentLevel);
  }

  void printTestDecl(AP(TestDecl)& td, int ind)
  {
    indent(ind);
    cout << "Test\n";
    printCall(td->call, ind + indentLevel);
  }

  void printEnum(AP(Enum)& e, int ind)
  {
    indent(ind);
    cout << "Enum: \"" << e->name << "\":\n";
    if(!e)
    {
      cout << "Error: trying to print null Parser::Enum\n";
    }
    for(auto& item : e->items)
    {
      indent(ind + indentLevel);
      cout << item->name << ": ";
      if(item->value)
      {
        cout << item->value->val << '\n';
      }
      else
      {
        cout << "automatic\n";
      }
    }
  }

  void printBlock(AP(Block)& b, int ind)
  {
    indent(ind);
    cout << "Block\n";
    for(auto& s : b->statements)
    {
      printStatement(s, ind + indentLevel);
    }
  }

  void printVarDecl(AP(VarDecl)& vd, int ind)
  {
    indent(ind);
    cout << "Variable declaration\n";
    indent(ind + indentLevel);
    cout << "Name: " << vd->name << '\n';
    if(vd->type)
    {
      printTypeNT(vd->type, ind + indentLevel);
    }
    else
    {
      indent(ind + indentLevel);
      cout << "Type: auto\n";
    }
    if(vd->val)
    {
      indent(ind + indentLevel);
      cout << "Value:\n";
      printExpression(vd->val, ind + indentLevel);
    }
    else
    {
      indent(ind + indentLevel);
      cout << "Zero-initialized\n";
    }
  }

  void printVarAssign(AP(VarAssign)& va, int ind)
  {
    indent(ind);
    cout << "Variable assignment\n";
    indent(ind + indentLevel);
    cout << "Variable:\n";
    printMember(va->target, ind + indentLevel);
    indent(ind + indentLevel);
    cout << "Operator: " << va->op->getStr() << '\n';
    if(va->rhs)
    {
      indent(ind + indentLevel);
      cout << "Right-hand side:\n";
      printExpression(va->rhs, ind + indentLevel);
    }
  }

  void printPrint(AP(Print)& p, int ind)
  {
    indent(ind);
    cout << "Print\n";
    for(auto& e : p->exprs)
    {
      printExpression(e, ind + indentLevel);
    }
  }

  void printExpression(AP(Expression)& e, int ind)
  {
    indent(ind);
    cout << "Expression\n";
    if(e->e.is<AP(Call)>())
      printCall(e->e.get<AP(Call)>(), ind + indentLevel);
    else if(e->e.is<AP(Member)>())
      printMember(e->e.get<AP(Member)>(), ind + indentLevel);
    else if(e->e.is<AP(Expr1)>())
      printExpr1(e->e.get<AP(Expr1)>(), ind + indentLevel);
  }

  void printCall(AP(Call)& c, int ind)
  {
    indent(ind);
    cout << "Call\n";
    indent(ind + indentLevel);
    cout << "Function/Procedure: \n";
    printMember(c->callable, ind + indentLevel);
    indent(ind + indentLevel);
    cout << "Args:\n";
    for(auto& it : c->args)
    {
      printExpression(it, ind + indentLevel);
    }
  }

  void printArg(AP(Arg)& a, int ind)
  {
    indent(ind);
    cout << "Argument: ";
    if(a->haveName)
    {
      cout << '\"' << a->name << "\"\n";
    }
    else
    {
      cout << "unnamed\n";
    }
    if(a->t.is<AP(TypeNT)>())
      printTypeNT(a->t.get<AP(TypeNT)>(), ind + indentLevel);
    else if(a->t.is<AP(TraitType)>())
      printTraitType(a->t.get<AP(TraitType)>(), ind + indentLevel);
  }

  void printFuncDecl(AP(FuncDecl)& fd, int ind)
  {
    indent(ind);
    cout << "Func declaration: \"" << fd->name << "\"\n";
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(fd->retType, ind + indentLevel);
    indent(ind);
    if(fd->args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : fd->args)
    {
      printArg(it, ind + indentLevel);
    }
  }

  void printFuncDef(AP(FuncDef)& fd, int ind)
  {
    indent(ind);
    cout << "Func definition:\n";
    printMember(fd->name, ind + indentLevel);
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(fd->retType, ind + indentLevel);
    indent(ind);
    if(fd->args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : fd->args)
    {
      printArg(it, ind + indentLevel);
    }
    indent(ind);
    cout << "Body:\n";
    printBlock(fd->body, ind + indentLevel);
  }

  void printFuncType(AP(FuncType)& ft, int ind)
  {
    indent(ind);
    cout << "Func type:\n";
    printMember(ft->name, ind + indentLevel);
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(ft->retType, ind + indentLevel);
    indent(ind);
    if(ft->args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : ft->args)
    {
      printArg(it, ind + indentLevel);
    }
  }

  void printProcDecl(AP(ProcDecl)& pd, int ind)
  {
    indent(ind);
    cout << "Proc declaration: \"" << pd->name << "\"\n";
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(pd->retType, ind + indentLevel);
    indent(ind);
    if(pd->args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : pd->args)
    {
      printArg(it, ind + indentLevel);
    }
  }

  void printProcDef(AP(ProcDef)& pd, int ind)
  {
    indent(ind);
    cout << "Proc definition:\n";
    printMember(pd->name, ind + indentLevel);
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(pd->retType, ind + indentLevel);
    indent(ind);
    if(pd->args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : pd->args)
    {
      printArg(it, ind + indentLevel);
    }
    indent(ind);
    cout << "Body:\n";
    printBlock(pd->body, ind + indentLevel);
  }

  void printProcType(AP(ProcType)& pt, int ind)
  {
    indent(ind);
    cout << "Func type:\n";
    printMember(pt->name, ind + indentLevel);
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(pt->retType, ind + indentLevel);
    indent(ind);
    if(pt->args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : pt->args)
    {
      printArg(it, ind + indentLevel);
    }
  }

  void printStructDecl(AP(StructDecl)& sd, int ind)
  {
    indent(ind);  
    cout << "Struct \"" << sd->name << "\"\n";
    if(sd->traits.size())
    {
      indent(ind + indentLevel);
      cout << "Traits:\n";
      for(auto& it : sd->traits)
      {
        printMember(it, ind + indentLevel);
      }
    }
    indent(ind + indentLevel);
    cout << "Members:\n";
    for(auto it : sd->members)
    {
      if(it->compose)
      {
        indent(ind + indentLevel);
        cout << "(Composed)\n";
      }
      printScopedDecl(it->sd, ind + indentLevel);
    }
  }

  void printUnionDecl(AP(UnionDecl)& vd, int ind)
  {
    indent(ind);
    cout << "Union \"" << vd->name << "\"\n";
    for(auto& it : vd->types)
    {
      printTypeNT(it, ind + indentLevel);
    }
  }

  void printTraitDecl(AP(TraitDecl)& td, int ind)
  {
    indent(ind);
    cout << "Trait \"" << td->name << "\"\n";
    for(auto& it : td->members)
    {
      if(it.is<AP(FuncDecl)>())
        printFuncDecl(it.get<AP(FuncDecl)>(), ind + indentLevel);
      else if(it.is<AP(ProcDecl)>())
        printProcDecl(it.get<AP(ProcDecl)>(), ind + indentLevel);
    }
  }

  void printStructLit(AP(StructLit)& sl, int ind)
  {
    indent(ind);
    cout << "Struct/Array literal\n";
    for(auto& it : sl->vals)
    {
      printExpression(it, ind + indentLevel);
    }
  }

  void printMember(AP(Member)& m, int ind)
  {
    indent(ind);
    cout << "Member \"" << m->owner << "\":\n";
    if(m->mem)
    {
      printMember(m->mem, ind + indentLevel);
    }
  }

  void printTraitType(AP(TraitType)& tt, int ind)
  {
    indent(ind);
    cout << "TraitType \"" << tt->localName << "\", underlying:\n";
    printMember(tt->traitName, ind + indentLevel);
  }

  void printTupleTypeNT(AP(TupleTypeNT)& tt, int ind)
  {
    indent(ind);
    cout << "Tuple type, members:\n";
    for(auto& it : tt->members)
    {
      printTypeNT(it, ind + indentLevel);
    }
  }

  void printBoolLit(AP(BoolLit)& bl, int ind)
  {
    indent(ind);
    cout << "Bool lit: ";
    if(bl->val)
    {
      cout << "true\n";
    }
    else
    {
      cout << "false\n";
    }
  }

  void printExpr1(AP(Expr1)& e, int ind)
  {
    indent(ind);
    cout << "Expr1, head:\n";
    printExpr2(e->head, ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr1RHS(it, ind + indentLevel);
    }
  }

  void printExpr1RHS(AP(Expr1RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr1RHS (||)\n";
    printExpr2(e->rhs, ind + indentLevel);
  }

  void printExpr2(AP(Expr2)& e, int ind)
  {
    indent(ind);
    cout << "Expr2, head:\n";
    printExpr3(e->head, ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr2RHS(it, ind + indentLevel);
    }
  }

  void printExpr2RHS(AP(Expr2RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr2RHS (&&)\n";
    printExpr3(e->rhs, ind + indentLevel);
  }

  void printExpr3(AP(Expr3)& e, int ind)
  {
    indent(ind);
    cout << "Expr3, head:\n";
    printExpr4(e->head, ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr3RHS(it, ind + indentLevel);
    }
  }

  void printExpr3RHS(AP(Expr3RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr3RHS (|)\n";
    printExpr4(e->rhs, ind + indentLevel);
  }

  void printExpr4(AP(Expr4)& e, int ind)
  {
    indent(ind);
    cout << "Expr4, head:\n";
    printExpr5(e->head, ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr4RHS(it, ind + indentLevel);
    }
  }

  void printExpr4RHS(AP(Expr4RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr4RHS (^)\n";
    printExpr5(e->rhs, ind + indentLevel);
  }

  void printExpr5(AP(Expr5)& e, int ind)
  {
    indent(ind);
    cout << "Expr5, head:\n";
    printExpr6(e->head, ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr5RHS(it, ind + indentLevel);
    }
  }

  void printExpr5RHS(AP(Expr5RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr5RHS (&)\n";
    printExpr6(e->rhs, ind + indentLevel);
  }

  void printExpr6(AP(Expr6)& e, int ind)
  {
    indent(ind);
    cout << "Expr6, head:\n";
    printExpr7(e->head, ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr6RHS(it, ind + indentLevel);
    }
  }

  void printExpr6RHS(AP(Expr6RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr6RHS (" << operatorTable[e->op] << ")\n";
    printExpr7(e->rhs, ind + indentLevel);
  }

  void printExpr7(AP(Expr7)& e, int ind)
  {
    indent(ind);
    cout << "Expr7, head:\n";
    printExpr8(e->head, ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr7RHS(it, ind + indentLevel);
    }
  }

  void printExpr7RHS(AP(Expr7RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr7RHS (" << operatorTable[e->op] << ")\n";
    printExpr8(e->rhs, ind + indentLevel);
  }

  void printExpr8(AP(Expr8)& e, int ind)
  {
    indent(ind);
    cout << "Expr8, head:\n";
    printExpr9(e->head, ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr8RHS(it, ind + indentLevel);
    }
  }

  void printExpr8RHS(AP(Expr8RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr8RHS (" << operatorTable[e->op] << ")\n";
    printExpr9(e->rhs, ind + indentLevel);
  }

  void printExpr9(AP(Expr9)& e, int ind)
  {
    indent(ind);
    cout << "Expr9, head:\n";
    printExpr10(e->head, ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr9RHS(it, ind + indentLevel);
    }
  }

  void printExpr9RHS(AP(Expr9RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr9RHS (" << operatorTable[e->op] << ")\n";
    printExpr10(e->rhs, ind + indentLevel);
  }

  void printExpr10(AP(Expr10)& e, int ind)
  {
    indent(ind);
    cout << "Expr10, head:\n";
    printExpr11(e->head, ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr10RHS(it, ind + indentLevel);
    }
  }

  void printExpr10RHS(AP(Expr10RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr10RHS (" << operatorTable[e->op] << ")\n";
    printExpr11(e->rhs, ind + indentLevel);
  }

  void printExpr11(AP(Expr11)& e, int ind)
  {
    typedef Expr11::UnaryExpr UE;
    indent(ind);
    cout << "Expr11\n";
    if(e->e.is<AP(Expr12)>())
    {
      printExpr12(e->e.get<AP(Expr12)>(), ind + indentLevel);
    }
    else if(e->e.is<UE>())
    {
      UE& ue = e->e.get<UE>();
      indent(ind);
      cout << "Unary expr, op: " << operatorTable[ue.op] << "\n";
      printExpr11(ue.rhs, ind + indentLevel);
    }
  }

  void printExpr12(AP(Expr12)& e, int ind)
  {
    indent(ind);
    if(e->e.is<IntLit*>())
    {
      cout << "Int literal: " << e->e.get<IntLit*>()->val << '\n';
    }
    else if(e->e.is<CharLit*>())
    {
      char c = e->e.get<CharLit*>()->val;
      if(isprint(c))
        cout << "Char literal: " << c << '\n';
      else
        printf("Char literal: %#02hhx\n", c);
    }
    else if(e->e.is<StrLit*>())
    {
      cout << "String literal: \"" << e->e.get<StrLit*>()->val << "\"\n";
    }
    else if(e->e.is<FloatLit*>())
    {
      printf("Float literal: %.3e\n", e->e.get<FloatLit*>()->val);
    }
    else if(e->e.is<AP(BoolLit)>())
    {
      printBoolLit(e->e.get<AP(BoolLit)>(), 0);
    }
    else if(e->e.is<AP(Expression)>())
    {
      printExpression(e->e.get<AP(Expression)>(), indentLevel);
    }
    else if(e->e.is<AP(Member)>())
    {
      printMember(e->e.get<AP(Member)>(), indentLevel);
    }
    else if(e->e.is<AP(StructLit)>())
    {
      printStructLit(e->e.get<AP(StructLit)>(), indentLevel);
    }
  }
}

