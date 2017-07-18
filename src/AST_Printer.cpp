#include "AST_Printer.hpp"

using namespace std;

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
      printScopedDecl(it.get(), ind + indentLevel);
    }
  }


  void printScopedDecl(ScopedDecl* m, int ind)
  {
    indent(ind);
    cout << "ScopedDecl\n";
    if(m->decl.is<AP(Module)>())
      printModule(m->decl.get<AP(Module)>().get(), ind + indentLevel);
    else if(m->decl.is<AP(VarDecl)>())
      printVarDecl(m->decl.get<AP(VarDecl)>().get(), ind + indentLevel);
    else if(m->decl.is<AP(StructDecl)>())
      printStructDecl(m->decl.get<AP(StructDecl)>().get(), ind + indentLevel);
    else if(m->decl.is<AP(UnionDecl)>())
      printUnionDecl(m->decl.get<AP(UnionDecl)>().get(), ind + indentLevel);
    else if(m->decl.is<AP(TraitDecl)>())
      printTraitDecl(m->decl.get<AP(TraitDecl)>().get(), ind + indentLevel);
    else if(m->decl.is<AP(Enum)>())
      printEnum(m->decl.get<AP(Enum)>().get(), ind + indentLevel);
    else if(m->decl.is<AP(Typedef)>())
      printTypedef(m->decl.get<AP(Typedef)>().get(), ind + indentLevel);
    else if(m->decl.is<AP(FuncDecl)>())
      printFuncDecl(m->decl.get<AP(FuncDecl)>().get(), ind + indentLevel);
    else if(m->decl.is<AP(FuncDef)>())
      printFuncDef(m->decl.get<AP(FuncDef)>().get(), ind + indentLevel);
    else if(m->decl.is<AP(ProcDecl)>())
      printProcDecl(m->decl.get<AP(ProcDecl)>().get(), ind + indentLevel);
    else if(m->decl.is<AP(ProcDef)>())
      printProcDef(m->decl.get<AP(ProcDef)>().get(), ind + indentLevel);
    else if(m->decl.is<AP(TestDecl)>())
      printTestDecl(m->decl.get<AP(TestDecl)>().get(), ind + indentLevel);
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
      printMember(t->t.get<AP(Member)>().get(), ind + indentLevel);
    }
    else if(t->t.is<AP(TupleTypeNT)>())
    {
      //tuple type, print indented on next line
      cout << '\n';
      printTupleTypeNT(t->t.get<AP(TupleTypeNT)>().get(), ind + indentLevel);
    }
    else if(t->t.is<AP(FuncTypeNT)>())
    {
      printFuncTypeNT(t->t.get<AP(FuncTypeNT)>().get(), ind + indentLevel);
    }
    else if(t->t.is<AP(ProcTypeNT)>())
    {
      printProcTypeNT(t->t.get<AP(ProcTypeNT)>().get(), ind + indentLevel);
    }
    else if(t->t.is<AP(TraitType)>())
    {
      printTraitType(t->t.get<AP(TraitType)>().get(), ind + indentLevel);
    }
  }

  void printStatement(Statement* s, int ind)
  {
    //statements don't need any extra printouts
    if(s->s.is<AP(ScopedDecl)>())
      printScopedDecl(s->s.get<AP(ScopedDecl)>().get(), ind);
    else if(s->s.is<AP(VarAssign)>())
      printVarAssign(s->s.get<AP(VarAssign)>().get(), ind);
    else if(s->s.is<AP(Print)>())
      printPrint(s->s.get<AP(Print)>().get(), ind);
    else if(s->s.is<AP(ExpressionNT)>())
      printExpressionNT(s->s.get<AP(ExpressionNT)>().get(), ind);
    else if(s->s.is<AP(Block)>())
      printBlock(s->s.get<AP(Block)>().get(), ind);
    else if(s->s.is<AP(Return)>())
      printReturn(s->s.get<AP(Return)>().get(), ind);
    else if(s->s.is<AP(Continue)>())
      printContinue(ind);
    else if(s->s.is<AP(Break)>())
      printBreak(ind);
    else if(s->s.is<AP(Switch)>())
      printSwitch(s->s.get<AP(Switch)>().get(), ind);
    else if(s->s.is<AP(For)>())
      printFor(s->s.get<AP(For)>().get(), ind);
    else if(s->s.is<AP(While)>())
      printWhile(s->s.get<AP(While)>().get(), ind);
    else if(s->s.is<AP(If)>())
      printIf(s->s.get<AP(If)>().get(), ind);
    else if(s->s.is<AP(Assertion)>())
      printAssertion(s->s.get<AP(Assertion)>().get(), ind);
    else if(s->s.is<AP(EmptyStatement)>())
      printEmptyStatement(ind);
    else if(s->s.is<AP(VarDecl)>())
      printVarDecl(s->s.get<AP(VarDecl)>().get(), ind);
  }

  void printTypedef(Typedef* t, int ind)
  {
    indent(ind);
    cout << "Typedef \"" << t->ident << "\"\n";
    printTypeNT(t->type.get(), ind + indentLevel);
  }

  void printReturn(Return* r, int ind)
  {
    indent(ind);
    cout << "Return\n";
    if(r->ex)
    {
      printExpressionNT(r->ex.get(), ind + 2);
    }
  }

  void printSwitch(Switch* s, int ind)
  {
    indent(ind);
    cout << "Switch\n";
    indent(ind + indentLevel);
    cout << "Value\n";
    printExpressionNT(s->sw.get(), ind + indentLevel);
    for(auto sc : s->cases)
    {
      indent(ind + indentLevel);
      cout << "Match value:\n";
      printExpressionNT(sc->matchVal.get(), ind + indentLevel);
      indent(ind + indentLevel);
      cout << "Match statement:\n";
      printStatement(sc->s.get(), ind + indentLevel);
    }
    if(s->defaultStatement)
    {
      indent(ind + indentLevel);
      cout << "Default statement:\n";
      printStatement(s->defaultStatement.get(), ind + indentLevel);
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
    if(f->f.is<AP(ForC)>())
    {
      cout << "C-style\n";
      auto& forC = f->f.get<AP(ForC)>();
      indent(ind + indentLevel);
      cout << "Initializer: ";
      if(forC->decl)
      {
        cout << '\n';
        printVarDecl(forC->decl.get(), ind + indentLevel);
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
        printExpressionNT(forC->condition.get(), ind + indentLevel);
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
        printVarAssign(forC->incr.get(), ind + indentLevel);
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
      printExpressionNT(forRange1->expr.get(), ind + indentLevel);
    }
    else if(f->f.is<AP(ForRange2)>())
    {
      cout << "[n1, n2) range\n";
      auto& forRange2 = f->f.get<AP(ForRange2)>();
      indent(ind + indentLevel);
      cout << "Lower bound:\n";
      printExpressionNT(forRange2->start.get(), ind + indentLevel);
      cout << "Upper bound:\n";
      printExpressionNT(forRange2->end.get(), ind + indentLevel);
    }
    else if(f->f.is<AP(ForArray)>())
    {
      cout << "container\n";
      auto& forArray = f->f.get<AP(ForArray)>();
      indent(ind + indentLevel);
      cout << "Container:\n";
      printExpressionNT(forArray->container.get(), ind + indentLevel);
    }
    indent(ind);
    cout << "Body:\n";
    printStatement(f->body.get(), ind + indentLevel);
  }

  void printWhile(While* w, int ind)
  {
    indent(ind);
    cout << "While\n";
    indent(ind + indentLevel);
    cout << "Condition:\n";
    printExpressionNT(w->cond.get(), ind + indentLevel);
    indent(ind + indentLevel);
    cout << "Body:\n";
    printStatement(w->body.get(), ind + indentLevel);
  }

  void printIf(If* i, int ind)
  {
    indent(ind);
    cout << "If\n";
    indent(ind + indentLevel);
    cout << "Condition\n";
    printExpressionNT(i->cond.get(), ind + indentLevel);
    indent(ind + indentLevel);
    cout << "If Body\n";
    printStatement(i->ifBody.get(), ind + indentLevel);
    if(i->elseBody)
    {
      indent(ind + indentLevel);
      cout << "Else Body\n";
      printStatement(i->elseBody.get(), ind + indentLevel);
    }
  }

  void printAssertion(Assertion* a, int ind)
  {
    indent(ind);
    cout << "Assertion\n";
    printExpressionNT(a->expr.get(), ind + indentLevel);
  }

  void printTestDecl(TestDecl* td, int ind)
  {
    indent(ind);
    cout << "Test\n";
    printCall(td->call.get(), ind + indentLevel);
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
      printStatement(s.get(), ind + indentLevel);
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
      printTypeNT(vd->type.get(), ind + indentLevel);
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
      printExpressionNT(vd->val.get(), ind + indentLevel);
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
    printExpressionNT(va->target.get(), ind + indentLevel);
    indent(ind + indentLevel);
    cout << "Operator: " << va->op->getStr() << '\n';
    if(va->rhs)
    {
      indent(ind + indentLevel);
      cout << "Assigned value:\n";
      printExpressionNT(va->rhs.get(), ind + indentLevel);
    }
  }

  void printPrint(Print* p, int ind)
  {
    indent(ind);
    cout << "Print\n";
    for(auto& e : p->exprs)
    {
      printExpressionNT(e.get(), ind + indentLevel);
    }
  }

  void printCall(Call* c, int ind)
  {
    indent(ind);
    cout << "Call\n";
    indent(ind + indentLevel);
    cout << "Function/Procedure: \n";
    printMember(c->callable.get(), ind + indentLevel);
    indent(ind + indentLevel);
    cout << "Args:\n";
    for(auto& it : c->args)
    {
      printExpressionNT(it.get(), ind + indentLevel);
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
    printTypeNT(a->type.get(), ind + indentLevel);
  }

  void printFuncDecl(FuncDecl* fd, int ind)
  {
    indent(ind);
    cout << "Func declaration: \"" << fd->name << "\"\n";
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(fd->type.retType.get(), ind + indentLevel);
    indent(ind);
    if(fd->type.args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : fd->type.args)
    {
      printArg(it.get(), ind + indentLevel);
    }
  }

  void printFuncDef(FuncDef* fd, int ind)
  {
    indent(ind);
    cout << "Func definition:\n";
    printMember(fd->name.get(), ind + indentLevel);
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(fd->type.retType.get(), ind + indentLevel);
    indent(ind);
    if(fd->type.args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : fd->type.args)
    {
      printArg(it.get(), ind + indentLevel);
    }
    indent(ind);
    cout << "Body:\n";
    printBlock(fd->body.get(), ind + indentLevel);
  }

  void printFuncTypeNT(FuncTypeNT* ft, int ind)
  {
    indent(ind);
    cout << "Func type:\n";
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(ft->retType.get(), ind + indentLevel);
    indent(ind);
    if(ft->args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : ft->args)
    {
      printArg(it.get(), ind + indentLevel);
    }
  }

  void printProcDecl(ProcDecl* pd, int ind)
  {
    indent(ind);
    cout << "Proc declaration: \"" << pd->name << "\"\n";
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(pd->type.retType.get(), ind + indentLevel);
    indent(ind);
    if(pd->type.args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : pd->type.args)
    {
      printArg(it.get(), ind + indentLevel);
    }
  }

  void printProcDef(ProcDef* pd, int ind)
  {
    indent(ind);
    cout << "Proc definition:\n";
    printMember(pd->name.get(), ind + indentLevel);
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(pd->type.retType.get(), ind + indentLevel);
    indent(ind);
    if(pd->type.args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : pd->type.args)
    {
      printArg(it.get(), ind + indentLevel);
    }
    indent(ind);
    cout << "Body:\n";
    printBlock(pd->body.get(), ind + indentLevel);
  }

  void printProcTypeNT(ProcTypeNT* pt, int ind)
  {
    indent(ind);
    cout << "Proc type:\n";
    indent(ind);
    cout << "Return type:\n";
    printTypeNT(pt->retType.get(), ind + indentLevel);
    indent(ind);
    if(pt->args.size() == 0)
      cout << "No args\n";
    else
      cout << "Args:\n";
    for(auto& it : pt->args)
    {
      printArg(it.get(), ind + indentLevel);
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
        printMember(it.get(), ind + indentLevel);
      }
    }
    indent(ind + indentLevel);
    cout << "Members:\n";
    for(auto it : sd->members)
    {
      bool staticData = it->sd->decl.is<AP(VarDecl)>() && it->sd->decl.get<AP(VarDecl)>()->isStatic;
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
      printScopedDecl(it->sd.get(), ind + indentLevel);
    }
  }

  void printUnionDecl(UnionDecl* vd, int ind)
  {
    indent(ind);
    cout << "Union \"" << vd->name << "\"\n";
    for(auto& it : vd->types)
    {
      printTypeNT(it.get(), ind + indentLevel);
    }
  }

  void printTraitDecl(TraitDecl* td, int ind)
  {
    indent(ind);
    cout << "Trait \"" << td->name << "\"\n";
    for(auto& it : td->members)
    {
      if(it.is<AP(FuncDecl)>())
        printFuncDecl(it.get<AP(FuncDecl)>().get(), ind + indentLevel);
      else if(it.is<AP(ProcDecl)>())
        printProcDecl(it.get<AP(ProcDecl)>().get(), ind + indentLevel);
    }
  }

  void printStructLit(StructLit* sl, int ind)
  {
    indent(ind);
    cout << "Struct/Array literal\n";
    for(auto& it : sl->vals)
    {
      printExpressionNT(it.get(), ind + indentLevel);
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
      printMember(mem.get(), ind + indentLevel);
    }
  }

  void printTupleTypeNT(TupleTypeNT* tt, int ind)
  {
    indent(ind);
    cout << "Tuple type, members:\n";
    for(auto& it : tt->members)
    {
      printTypeNT(it.get(), ind + indentLevel);
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
    printExpr2(e->head.get(), ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr1RHS(it.get(), ind + indentLevel);
    }
  }

  void printExpr1RHS(Expr1RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr1RHS (||)\n";
    printExpr2(e->rhs.get(), ind + indentLevel);
  }

  void printExpr2(Expr2* e, int ind)
  {
    indent(ind);
    cout << "Expr2, head:\n";
    printExpr3(e->head.get(), ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr2RHS(it.get(), ind + indentLevel);
    }
  }

  void printExpr2RHS(Expr2RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr2RHS (&&)\n";
    printExpr3(e->rhs.get(), ind + indentLevel);
  }

  void printExpr3(Expr3* e, int ind)
  {
    indent(ind);
    cout << "Expr3, head:\n";
    printExpr4(e->head.get(), ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr3RHS(it.get(), ind + indentLevel);
    }
  }

  void printExpr3RHS(Expr3RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr3RHS (|)\n";
    printExpr4(e->rhs.get(), ind + indentLevel);
  }

  void printExpr4(Expr4* e, int ind)
  {
    indent(ind);
    cout << "Expr4, head:\n";
    printExpr5(e->head.get(), ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr4RHS(it.get(), ind + indentLevel);
    }
  }

  void printExpr4RHS(Expr4RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr4RHS (^)\n";
    printExpr5(e->rhs.get(), ind + indentLevel);
  }

  void printExpr5(Expr5* e, int ind)
  {
    indent(ind);
    cout << "Expr5, head:\n";
    printExpr6(e->head.get(), ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr5RHS(it.get(), ind + indentLevel);
    }
  }

  void printExpr5RHS(Expr5RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr5RHS (&)\n";
    printExpr6(e->rhs.get(), ind + indentLevel);
  }

  void printExpr6(Expr6* e, int ind)
  {
    indent(ind);
    cout << "Expr6, head:\n";
    printExpr7(e->head.get(), ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr6RHS(it.get(), ind + indentLevel);
    }
  }

  void printExpr6RHS(Expr6RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr6RHS (" << operatorTable[e->op] << ")\n";
    printExpr7(e->rhs.get(), ind + indentLevel);
  }

  void printExpr7(Expr7* e, int ind)
  {
    indent(ind);
    cout << "Expr7, head:\n";
    printExpr8(e->head.get(), ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr7RHS(it.get(), ind + indentLevel);
    }
  }

  void printExpr7RHS(Expr7RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr7RHS (" << operatorTable[e->op] << ")\n";
    printExpr8(e->rhs.get(), ind + indentLevel);
  }

  void printExpr8(Expr8* e, int ind)
  {
    indent(ind);
    cout << "Expr8, head:\n";
    printExpr9(e->head.get(), ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr8RHS(it.get(), ind + indentLevel);
    }
  }

  void printExpr8RHS(Expr8RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr8RHS (" << operatorTable[e->op] << ")\n";
    printExpr9(e->rhs.get(), ind + indentLevel);
  }

  void printExpr9(Expr9* e, int ind)
  {
    indent(ind);
    cout << "Expr9, head:\n";
    printExpr10(e->head.get(), ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr9RHS(it.get(), ind + indentLevel);
    }
  }

  void printExpr9RHS(Expr9RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr9RHS (" << operatorTable[e->op] << ")\n";
    printExpr10(e->rhs.get(), ind + indentLevel);
  }

  void printExpr10(Expr10* e, int ind)
  {
    indent(ind);
    cout << "Expr10, head:\n";
    printExpr11(e->head.get(), ind + indentLevel);
    if(e->tail.size())
    {
      indent(ind);
      cout << "tail:\n";
    }
    for(auto& it : e->tail)
    {
      printExpr10RHS(it.get(), ind + indentLevel);
    }
  }

  void printExpr10RHS(Expr10RHS* e, int ind)
  {
    indent(ind);
    cout << "Expr10RHS (" << operatorTable[e->op] << ")\n";
    printExpr11(e->rhs.get(), ind + indentLevel);
  }

  void printExpr11(Expr11* e, int ind)
  {
    typedef Expr11::UnaryExpr UE;
    indent(ind);
    cout << "Expr11\n";
    if(e->e.is<AP(Expr12)>())
    {
      printExpr12(e->e.get<AP(Expr12)>().get(), ind + indentLevel);
    }
    else if(e->e.is<UE>())
    {
      UE& ue = e->e.get<UE>();
      indent(ind);
      cout << "Unary expr, op: " << operatorTable[ue.op] << "\n";
      printExpr11(ue.rhs.get(), ind + indentLevel);
    }
  }

  void printExpr12(Expr12* e, int ind)
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
      printBoolLit(e->e.get<AP(BoolLit)>().get(), 0);
    }
    else if(e->e.is<AP(ExpressionNT)>())
    {
      printExpressionNT(e->e.get<AP(ExpressionNT)>().get(), indentLevel);
    }
    else if(e->e.is<AP(Member)>())
    {
      printMember(e->e.get<AP(Member)>().get(), indentLevel);
    }
    else if(e->e.is<AP(StructLit)>())
    {
      printStructLit(e->e.get<AP(StructLit)>().get(), indentLevel);
    }
    else if(e->e.is<Parser::Expr12::ArrayIndex>())
    {
      auto& ai = e->e.get<Parser::Expr12::ArrayIndex>();
      cout << "Array/tuple index\n";
      indent(ind);
      cout << "Array or tuple expression:\n";
      printExpr12(ai.arr.get(), ind + 1);
      indent(ind);
      cout << "Index:\n";
      printExpressionNT(ai.index.get(), ind + 1);
    }
  }
}

