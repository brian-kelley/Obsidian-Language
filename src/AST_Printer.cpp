#include "AST_Printer.hpp"

void printAST(Parser::Module* ast)
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

  void printModule(Module* m, int ind)
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

  void printScopedDecl(ScopedDecl* m, int ind)
  {
    indent(ind);
    cout << "ScopedDecl\n";
    if(m->decl.is<Module*>())
      printModule(m->decl.get<Module*>(), ind + indentLevel);
    else if(m->decl.is<VarDecl*>())
      printVarDecl(m->decl.get<VarDecl*>(), ind + indentLevel);
    else if(m->decl.is<StructDecl*>())
      printStructDecl(m->decl.get<StructDecl*>(), ind + indentLevel);
    else if(m->decl.is<UnionDecl*>())
      printUnionDecl(m->decl.get<UnionDecl*>(), ind + indentLevel);
    else if(m->decl.is<TraitDecl*>())
      printTraitDecl(m->decl.get<TraitDecl*>(), ind + indentLevel);
    else if(m->decl.is<Enum*>())
      printEnum(m->decl.get<Enum*>(), ind + indentLevel);
    else if(m->decl.is<Typedef*>())
      printTypedef(m->decl.get<Typedef*>(), ind + indentLevel);
    else if(m->decl.is<FuncDecl*>())
      printFuncDecl(m->decl.get<FuncDecl*>(), ind + indentLevel);
    else if(m->decl.is<FuncDef*>())
      printFuncDef(m->decl.get<FuncDef*>(), ind + indentLevel);
    else if(m->decl.is<ProcDecl*>())
      printProcDecl(m->decl.get<ProcDecl*>(), ind + indentLevel);
    else if(m->decl.is<ProcDef*>())
      printProcDef(m->decl.get<ProcDef*>(), ind + indentLevel);
    else if(m->decl.is<TestDecl*>())
      printTestDecl(m->decl.get<TestDecl*>(), ind + indentLevel);
  }

  void printTypeNT(Parser::TypeNT* t, int ind)
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
        case TypeNT::Prim::BYTE:
          cout << "byte";
          break;
        case TypeNT::Prim::UBYTE:
          cout << "ubyte";
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
        default:
          cout << "invalid primitive";
      }
      cout << '\n';
    }
    else if(t->t.is<Member*>())
    {
      //member, print indented on next line
      cout << '\n';
      printMember(t->t.get<Member*>(), ind + indentLevel);
    }
    else if(t->t.is<TupleTypeNT*>())
    {
      //tuple type, print indented on next line
      cout << '\n';
      printTupleTypeNT(t->t.get<TupleTypeNT*>(), ind + indentLevel);
    }
    else if(t->t.is<FuncTypeNT*>())
    {
      printFuncTypeNT(t->t.get<FuncTypeNT*>(), ind + indentLevel);
    }
    else if(t->t.is<ProcTypeNT*>())
    {
      printProcTypeNT(t->t.get<ProcTypeNT*>(), ind + indentLevel);
    }
    else if(t->t.is<TraitType*>())
    {
      printTraitType(t->t.get<TraitType*>(), ind + indentLevel);
    }
  }

  void printStatementNT(StatementNT* s, int ind)
  {
    //statements don't need any extra printouts
    if(s->s.is<ScopedDecl*>())
      printScopedDecl(s->s.get<ScopedDecl*>(), ind);
    else if(s->s.is<VarAssign*>())
      printVarAssign(s->s.get<VarAssign*>(), ind);
    else if(s->s.is<PrintNT*>())
      printPrintNT(s->s.get<PrintNT*>(), ind);
    else if(s->s.is<CallNT*>())
      printCallNT(s->s.get<CallNT*>(), ind);
    else if(s->s.is<Block*>())
      printBlock(s->s.get<Block*>(), ind);
    else if(s->s.is<Return*>())
      printReturn(s->s.get<Return*>(), ind);
    else if(s->s.is<Continue*>())
      printContinue(ind);
    else if(s->s.is<Break*>())
      printBreak(ind);
    else if(s->s.is<Switch*>())
      printSwitch(s->s.get<Switch*>(), ind);
    else if(s->s.is<For*>())
      printFor(s->s.get<For*>(), ind);
    else if(s->s.is<While*>())
      printWhile(s->s.get<While*>(), ind);
    else if(s->s.is<If*>())
      printIf(s->s.get<If*>(), ind);
    else if(s->s.is<Assertion*>())
      printAssertion(s->s.get<Assertion*>(), ind);
    else if(s->s.is<EmptyStatement*>())
      printEmptyStatement(ind);
  }

  void printTypedef(Typedef* t, int ind)
  {
    indent(ind);
    cout << "Typedef \"" << t->ident << "\"\n";
    printTypeNT(t->type, ind + indentLevel);
  }

  void printReturn(Return* r, int ind)
  {
    indent(ind);
    cout << "Return\n";
    if(r->ex)
    {
      printExpressionNT(r->ex, ind + 2);
    }
  }

  void printSwitch(Switch* s, int ind)
  {
    indent(ind);
    cout << "Switch\n";
    indent(ind + indentLevel);
    cout << "Value\n";
    printExpressionNT(s->sw, ind + indentLevel);
    for(auto sc : s->cases)
    {
      indent(ind + indentLevel);
      cout << "Match value:\n";
      printExpressionNT(sc->matchVal, ind + indentLevel);
      indent(ind + indentLevel);
      cout << "Match statement:\n";
      printStatementNT(sc->s, ind + indentLevel);
    }
    if(s->defaultStatement)
    {
      indent(ind + indentLevel);
      cout << "Default statement:\n";
      printStatementNT(s->defaultStatement, ind + indentLevel);
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

  void printFor(For* f, int ind)
  {
    indent(ind);
    cout << "For ";
    if(f->f.is<ForC*>())
    {
      cout << "C-style\n";
      auto& forC = f->f.get<ForC*>();
      indent(ind + indentLevel);
      cout << "Initializer: ";
      if(forC->decl)
      {
        cout << '\n';
        printStatementNT(forC->decl, ind + indentLevel);
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
        printExpressionNT(forC->condition, ind + indentLevel);
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
        printStatementNT(forC->incr, ind + indentLevel);
      }
      else
      {
        cout << "none\n";
      }
    }
    else if(f->f.is<ForRange1*>())
    {
      cout << "[0, n) range\n";
      auto& forRange1 = f->f.get<ForRange1*>();
      indent(ind + indentLevel);
      cout << "Upper bound:\n";
      printExpressionNT(forRange1->expr, ind + indentLevel);
    }
    else if(f->f.is<ForRange2*>())
    {
      cout << "[n1, n2) range\n";
      auto& forRange2 = f->f.get<ForRange2*>();
      indent(ind + indentLevel);
      cout << "Lower bound:\n";
      printExpressionNT(forRange2->start, ind + indentLevel);
      cout << "Upper bound:\n";
      printExpressionNT(forRange2->end, ind + indentLevel);
    }
    indent(ind);
    cout << "Body:\n";
    printBlock(f->body, ind + indentLevel);
  }

  void printWhile(While* w, int ind)
  {
    indent(ind);
    cout << "While\n";
    indent(ind + indentLevel);
    cout << "Condition:\n";
    printExpressionNT(w->cond, ind + indentLevel);
    indent(ind + indentLevel);
    cout << "Body:\n";
    printBlock(w->body, ind + indentLevel);
  }

  void printIf(If* i, int ind)
  {
    indent(ind);
    cout << "If\n";
    indent(ind + indentLevel);
    cout << "Condition\n";
    printExpressionNT(i->cond, ind + indentLevel);
    indent(ind + indentLevel);
    cout << "If Body\n";
    printStatementNT(i->ifBody, ind + indentLevel);
    if(i->elseBody)
    {
      indent(ind + indentLevel);
      cout << "Else Body\n";
      printStatementNT(i->elseBody, ind + indentLevel);
    }
  }

  void printAssertion(Assertion* a, int ind)
  {
    indent(ind);
    cout << "Assertion\n";
    printExpressionNT(a->expr, ind + indentLevel);
  }

  void printTestDecl(TestDecl* td, int ind)
  {
    indent(ind);
    cout << "Test\n";
    printCallNT(td->call, ind + indentLevel);
  }

  void printEnum(Enum* e, int ind)
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

  void printBlock(Block* b, int ind)
  {
    indent(ind);
    cout << "Block\n";
    for(auto& s : b->statements)
    {
      printStatementNT(s, ind + indentLevel);
    }
  }

  void printVarDecl(VarDecl* vd, int ind)
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
      printExpressionNT(vd->val, ind + indentLevel);
    }
    else
    {
      indent(ind + indentLevel);
      cout << "Zero-initialized\n";
    }
  }

  void printVarAssign(VarAssign* va, int ind)
  {
    indent(ind);
    cout << "Variable assignment\n";
    indent(ind + indentLevel);
    cout << "L-value:\n";
    printExpressionNT(va->target, ind + indentLevel);
    indent(ind + indentLevel);
    cout << "R-value:\n";
    printExpressionNT(va->rhs, ind + indentLevel);
  }

  void printPrintNT(PrintNT* p, int ind)
  {
    indent(ind);
    cout << "PrintNT\n";
    for(auto& e : p->exprs)
    {
      printExpressionNT(e, ind + indentLevel);
    }
  }

  void printCallNT(CallNT* c, int ind)
  {
    indent(ind);
    cout << "CallNT\n";
    indent(ind + indentLevel);
    cout << "Function/Procedure: \n";
    printMember(c->callable, ind + indentLevel);
    indent(ind + indentLevel);
    cout << "Args:\n";
    for(auto& it : c->args)
    {
      printExpressionNT(it, ind + indentLevel);
    }
  }

  void printArg(Arg* a, int ind)
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
    printTypeNT(a->type, ind + indentLevel);
  }

  void printFuncDecl(FuncDecl* fd, int ind)
  {
    indent(ind);
    cout << "Func declaration: \"" << fd->name << "\"\n";
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(fd->type.retType, ind + indentLevel);
    indent(ind);
    if(fd->type.args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : fd->type.args)
    {
      printArg(it, ind + indentLevel);
    }
  }

  void printFuncDef(FuncDef* fd, int ind)
  {
    indent(ind);
    cout << "Func definition: " << fd->name << '\n';
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(fd->type.retType, ind + indentLevel);
    indent(ind);
    if(fd->type.args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : fd->type.args)
    {
      printArg(it, ind + indentLevel);
    }
    indent(ind);
    cout << "Body:\n";
    printBlock(fd->body, ind + indentLevel);
  }

  void printFuncTypeNT(FuncTypeNT* ft, int ind)
  {
    indent(ind);
    cout << "Func type:\n";
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

  void printProcDecl(ProcDecl* pd, int ind)
  {
    indent(ind);
    cout << "Proc declaration: \"" << pd->name << "\"\n";
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(pd->type.retType, ind + indentLevel);
    indent(ind);
    if(pd->type.args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : pd->type.args)
    {
      printArg(it, ind + indentLevel);
    }
  }

  void printProcDef(ProcDef* pd, int ind)
  {
    indent(ind);
    cout << "Proc definition: " << pd->name << '\n';
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(pd->type.retType, ind + indentLevel);
    indent(ind);
    if(pd->type.args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : pd->type.args)
    {
      printArg(it, ind + indentLevel);
    }
    indent(ind);
    cout << "Body:\n";
    printBlock(pd->body, ind + indentLevel);
  }

  void printProcTypeNT(ProcTypeNT* pt, int ind)
  {
    indent(ind);
    cout << "Proc type:\n";
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

  void printStructDecl(StructDecl* sd, int ind)
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
      bool staticData = it->sd->decl.is<VarDecl*>() && it->sd->decl.get<VarDecl*>()->isStatic;
      if(it->compose || staticData)
      {
        indent(ind + indentLevel);
        //note: a semantically valid program can't have any member be both
        //composed and static, but semantics haven't been checked yet
        if(it->compose)
        {
          cout << "(Composed) ";
        }
        if(staticData)
        {
          cout << "(Static)";
        }
        cout << '\n';
      }
      printScopedDecl(it->sd, ind + indentLevel);
    }
  }

  void printUnionDecl(UnionDecl* vd, int ind)
  {
    indent(ind);
    cout << "Union \"" << vd->name << "\"\n";
    for(auto& it : vd->types)
    {
      printTypeNT(it, ind + indentLevel);
    }
  }

  void printTraitDecl(TraitDecl* td, int ind)
  {
    indent(ind);
    cout << "Trait \"" << td->name << "\"\n";
    for(auto& it : td->members)
    {
      if(it.is<FuncDecl*>())
        printFuncDecl(it.get<FuncDecl*>(), ind + indentLevel);
      else if(it.is<ProcDecl*>())
        printProcDecl(it.get<ProcDecl*>(), ind + indentLevel);
    }
  }

  void printStructLit(StructLit* sl, int ind)
  {
    indent(ind);
    cout << "Struct/Array literal\n";
    for(auto& it : sl->vals)
    {
      printExpressionNT(it, ind + indentLevel);
    }
  }

  void printMember(Member* m, int ind)
  {
    indent(ind);
    cout << "Compound identifier: " << *m << '\n';
  }

  void printTraitType(TraitType* tt, int ind)
  {
    indent(ind);
    cout << "TraitType \"" << tt->localName << "\", traits:\n";
    for(auto& mem : tt->traits)
    {
      printMember(mem, ind + indentLevel);
    }
  }

  void printTupleTypeNT(TupleTypeNT* tt, int ind)
  {
    indent(ind);
    cout << "Tuple type, members:\n";
    for(auto& it : tt->members)
    {
      printTypeNT(it, ind + indentLevel);
    }
  }

  void printBoolLit(BoolLit* bl, int ind)
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

  void printExpressionNT(ExpressionNT* e, int ind)
  {
    printExpr1(e, ind);
  }

  void printExpr1(Expr1* e, int ind)
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

  void printExpr1RHS(Expr1RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr1RHS (||)\n";
    printExpr2(e->rhs, ind + indentLevel);
  }

  void printExpr2(Expr2* e, int ind)
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

  void printExpr2RHS(Expr2RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr2RHS (&&)\n";
    printExpr3(e->rhs, ind + indentLevel);
  }

  void printExpr3(Expr3* e, int ind)
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

  void printExpr3RHS(Expr3RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr3RHS (|)\n";
    printExpr4(e->rhs, ind + indentLevel);
  }

  void printExpr4(Expr4* e, int ind)
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

  void printExpr4RHS(Expr4RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr4RHS (^)\n";
    printExpr5(e->rhs, ind + indentLevel);
  }

  void printExpr5(Expr5* e, int ind)
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

  void printExpr5RHS(Expr5RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr5RHS (&)\n";
    printExpr6(e->rhs, ind + indentLevel);
  }

  void printExpr6(Expr6* e, int ind)
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

  void printExpr6RHS(Expr6RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr6RHS (" << operatorTable[e->op] << ")\n";
    printExpr7(e->rhs, ind + indentLevel);
  }

  void printExpr7(Expr7* e, int ind)
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

  void printExpr7RHS(Expr7RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr7RHS (" << operatorTable[e->op] << ")\n";
    printExpr8(e->rhs, ind + indentLevel);
  }

  void printExpr8(Expr8* e, int ind)
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

  void printExpr8RHS(Expr8RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr8RHS (" << operatorTable[e->op] << ")\n";
    printExpr9(e->rhs, ind + indentLevel);
  }

  void printExpr9(Expr9* e, int ind)
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

  void printExpr9RHS(Expr9RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr9RHS (" << operatorTable[e->op] << ")\n";
    printExpr10(e->rhs, ind + indentLevel);
  }

  void printExpr10(Expr10* e, int ind)
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

  void printExpr10RHS(Expr10RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr10RHS (" << operatorTable[e->op] << ")\n";
    printExpr11(e->rhs, ind + indentLevel);
  }

  void printExpr11(Expr11* e, int ind)
  {
    typedef Expr11::UnaryExpr UE;
    indent(ind);
    cout << "Expr11\n";
    if(e->e.is<Expr12*>())
    {
      printExpr12(e->e.get<Expr12*>(), ind + indentLevel);
    }
    else if(e->e.is<UE>())
    {
      UE& ue = e->e.get<UE>();
      indent(ind);
      cout << "Unary expr, op: " << operatorTable[ue.op] << "\n";
      printExpr11(ue.rhs, ind + indentLevel);
    }
  }

  void printExpr12(Expr12* e, int ind)
  {
    if(e->e.is<IntLit*>())
    {
      indent(ind);
      cout << "Int literal: " << e->e.get<IntLit*>()->val << '\n';
    }
    else if(e->e.is<CharLit*>())
    {
      indent(ind);
      char c = e->e.get<CharLit*>()->val;
      if(isprint(c))
        cout << "Char literal: " << c << '\n';
      else
        printf("Char literal: %#02hhx\n", c);
    }
    else if(e->e.is<StrLit*>())
    {
      indent(ind);
      cout << "String literal: \"" << e->e.get<StrLit*>()->val << "\"\n";
    }
    else if(e->e.is<FloatLit*>())
    {
      indent(ind);
      printf("Float literal: %.3e\n", e->e.get<FloatLit*>()->val);
    }
    else if(e->e.is<BoolLit*>())
    {
      indent(ind);
      printBoolLit(e->e.get<BoolLit*>(), 0);
    }
    else if(e->e.is<ExpressionNT*>())
    {
      printExpressionNT(e->e.get<ExpressionNT*>(), ind + 1);
    }
    else if(e->e.is<Member*>())
    {
      printMember(e->e.get<Member*>(), ind + 1);
    }
    else if(e->e.is<StructLit*>())
    {
      printStructLit(e->e.get<StructLit*>(), ind + 1);
    }
    else if(e->e.is<Parser::Expr12::ArrayIndex>())
    {
      indent(ind);
      auto& ai = e->e.get<Parser::Expr12::ArrayIndex>();
      cout << "Array/tuple index\n";
      indent(ind);
      cout << "Array or tuple expression:\n";
      printExpr12(ai.arr, ind + 1);
      indent(ind);
      cout << "Index:\n";
      printExpressionNT(ai.index, ind + 1);
    }
  }
}

