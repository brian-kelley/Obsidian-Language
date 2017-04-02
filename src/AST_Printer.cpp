#include "AST_Printer.hpp"

using namespace std;

void printAST(UP(Parser::ModuleDef)& ast)
{
  try
  {
    AstPrinter::printModuleDef(ast, 0);
  }
  catch(exception& e)
  {
    cout << "Error printing AST: " << e.what() << '\n';
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

  void printModule(UP(Module)& m, int ind)
  {
    indent(ind);
    cout << "Module \"" << m->name << "\"\n";
    printModuleDef(m->def, ind + indentLevel);
  }

  void printModuleDef(UP(ModuleDef)& m, int ind)
  {
    indent(ind);
    cout << "ModuleDef\n";
    for(auto& d : m->decls)
    {
      printScopedDecl(d, ind + indentLevel);
    }
  }

  void printScopedDecl(UP(ScopedDecl)& m, int ind)
  {
    indent(ind);
    cout << "ScopedDecl\n";
    switch(m->decl.which())
    {
      case 1:
        printModule(m->decl.get<UP(Module)>(), ind + indentLevel);
        break;
      case 2:
        printVarDecl(m->decl.get<UP(VarDecl)>(), ind + indentLevel);
        break;
      case 3:
        printStructDecl(m->decl.get<UP(StructDecl)>(), ind + indentLevel);
        break;
      case 4:
        printVariantDecl(m->decl.get<UP(VariantDecl)>(), ind + indentLevel);
        break;
      case 5:
        printTraitDecl(m->decl.get<UP(TraitDecl)>(), ind + indentLevel);
        break;
      case 6:
        printEnum(m->decl.get<UP(Enum)>(), ind + indentLevel);
        break;
      case 7:
        printTypedef(m->decl.get<UP(Typedef)>(), ind + indentLevel);
        break;
      case 8:
        printFuncDecl(m->decl.get<UP(FuncDecl)>(), ind + indentLevel);
        break;
      case 9:
        printFuncDef(m->decl.get<UP(FuncDef)>(), ind + indentLevel);
        break;
      case 10:
        printProcDecl(m->decl.get<UP(ProcDecl)>(), ind + indentLevel);
        break;
      case 11:
        printProcDef(m->decl.get<UP(ProcDef)>(), ind + indentLevel);
        break;
      case 12:
        printTestDecl(m->decl.get<UP(TestDecl)>(), ind + indentLevel);
        break;
      default:;
    }
  }

  void printType(AutoPtr<Parser::Type>& t, int ind)
  {
    indent(ind);
    cout << "Type: ";
    if(t->arrayDims)
    {
      cout << t->arrayDims << "-dim array of ";
    }
    switch(t->t.which())
    {
      case 1:
      {
        //primitive
        Type::Prim p = t->t.get<Type::Prim>();
        cout << "primitive ";
        switch(p)
        {
          case Type::Prim::BOOL:
            cout << "bool";
            break;
          case Type::Prim::CHAR:
            cout << "char";
            break;
          case Type::Prim::UCHAR:
            cout << "uchar";
            break;
          case Type::Prim::SHORT:
            cout << "short";
            break;
          case Type::Prim::USHORT:
            cout << "ushort";
            break;
          case Type::Prim::INT:
            cout << "int";
            break;
          case Type::Prim::UINT:
            cout << "uint";
            break;
          case Type::Prim::LONG:
            cout << "long";
            break;
          case Type::Prim::ULONG:
            cout << "ulong";
            break;
          case Type::Prim::FLOAT:
            cout << "float";
            break;
          case Type::Prim::DOUBLE:
            cout << "double";
            break;
          case Type::Prim::STRING:
            cout << "string";
            break;
          default:
            cout << "invalid primitive";
        }
        cout << '\n';
        break;
      }
      case 2:
        //member, print indented on next line
        cout << '\n';
        printMember(t->t.get<UP(Member)>(), ind + indentLevel);
        break;
      case 3:
        //tuple type, print indented on next line
        cout << '\n';
        printTupleType(t->t.get<UP(TupleType)>(), ind + indentLevel);
        break;
      default:;
    }
  }

  void printStatement(UP(Statement)& s, int ind)
  {
    //statements don't need any extra printouts
    switch(s->s.which())
    {
      case 1:
        printScopedDecl(s->s.get<UP(ScopedDecl)>(), ind);
        break;
      case 2:
        printVarAssign(s->s.get<UP(VarAssign)>(), ind);
        break;
      case 3:
        printPrint(s->s.get<UP(Print)>(), ind);
        break;
      case 4:
        printExpression(s->s.get<UP(Expression)>(), ind);
        break;
      case 5:
        printBlock(s->s.get<UP(Block)>(), ind);
        break;
      case 6:
        printReturn(s->s.get<UP(Return)>(), ind);
        break;
      case 7:
        printContinue(ind);
        break;
      case 8:
        printBreak(ind);
        break;
      case 9:
        printSwitch(s->s.get<UP(Switch)>(), ind);
        break;
      case 10:
        printFor(s->s.get<UP(For)>(), ind);
        break;
      case 11:
        printWhile(s->s.get<UP(While)>(), ind);
        break;
      case 12:
        printIf(s->s.get<UP(If)>(), ind);
        break;
      case 13:
        printUsing(s->s.get<UP(Using)>(), ind);
        break;
      case 14:
        printAssertion(s->s.get<UP(Assertion)>(), ind);
        break;
      case 15:
        printEmptyStatement(ind);
        break;
      case 16:
        printVarDecl(s->s.get<UP(VarDecl)>(), ind);
      default:;
    }
  }

  void printTypedef(UP(Typedef)& t, int ind)
  {
    indent(ind);
    cout << "Typedef\n";
    printType(t->type, ind + indentLevel);
    indent(ind + indentLevel);
    cout << "Name: " << t->ident << '\n';
  }

  void printReturn(UP(Return)& r, int ind)
  {
    indent(ind);
    cout << "Return\n";
    if(r->ex)
      printExpression(r->ex, ind + 2);
  }

  void printSwitch(UP(Switch)& s, int ind)
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

  void printFor(UP(For)& f, int ind)
  {
    indent(ind);
    cout << "For ";
    switch(f->f.which())
    {
      case 1:
      {
        cout << "C-style\n";
        auto& forC = f->f.get<UP(ForC)>();
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
        break;
      }
      case 2:
      {
        cout << "[0, n) range\n";
        auto& forRange1 = f->f.get<UP(ForRange1)>();
        indent(ind + indentLevel);
        cout << "Upper bound:\n";
        printExpression(forRange1->expr, ind + indentLevel);
        break;
      }
      case 3:
      {
        cout << "[n1, n2) range\n";
        auto& forRange2 = f->f.get<UP(ForRange2)>();
        indent(ind + indentLevel);
        cout << "Lower bound:\n";
        printExpression(forRange2->start, ind + indentLevel);
        cout << "Upper bound:\n";
        printExpression(forRange2->end, ind + indentLevel);
        break;
      }
      case 4:
      {
        cout << "container\n";
        auto& forArray = f->f.get<UP(ForArray)>();
        indent(ind + indentLevel);
        cout << "Container:\n";
        printExpression(forArray->container, ind + indentLevel);
        break;
      }
      default:;
    }
    indent(ind);
    cout << "Body:\n";
    printStatement(f->body, ind + indentLevel);
  }

  void printWhile(UP(While)& w, int ind)
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

  void printIf(UP(If)& i, int ind)
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

  void printUsing(UP(Using)& u, int ind)
  {
    indent(ind);
    cout << "Using\n";
    printMember(u->mem, ind);
  }

  void printAssertion(UP(Assertion)& a, int ind)
  {
    indent(ind);
    cout << "Assertion\n";
    printExpression(a->expr, ind + indentLevel);
  }

  void printTestDecl(UP(TestDecl)& td, int ind)
  {
    indent(ind);
    cout << "Test\n";
    printCall(td->call, ind + indentLevel);
  }

  void printEnum(UP(Enum)& e, int ind)
  {
    indent(ind);
    cout << "Enum: \"" << e->name << "\":\n";
    for(auto& item : e->items)
    {
      indent(ind + indentLevel);
      cout << item->name  << ": ";
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

  void printBlock(UP(Block)& b, int ind)
  {
    indent(ind);
    cout << "Block\n";
    for(auto& s : b->statements)
    {
      printStatement(s, ind + indentLevel);
    }
  }

  void printVarDecl(UP(VarDecl)& vd, int ind)
  {
    indent(ind);
    cout << "Variable declaration\n";
    indent(ind + indentLevel);
    cout << "Name: " << vd->name << '\n';
    if(vd->type)
    {
      printType(vd->type, ind + indentLevel);
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

  void printVarAssign(UP(VarAssign)& va, int ind)
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

  void printPrint(UP(Print)& p, int ind)
  {
    indent(ind);
    cout << "Print\n";
    for(auto& e : p->exprs)
    {
      printExpression(e, ind + indentLevel);
    }
  }

  void printExpression(UP(Expression)& e, int ind)
  {
    indent(ind);
    cout << "Expression\n";
    switch(e->e.which())
    {
      case 1:
        //call
        printCall(e->e.get<UP(Call)>(), ind + indentLevel);
        break;
      case 2:
        //member
        printMember(e->e.get<UP(Member)>(), ind + indentLevel);
        break;
      case 3:
        //expr1
        printExpr1(e->e.get<UP(Expr1)>(), ind + indentLevel);
        break;
      default:;
    }
  }

  void printCall(UP(Call)& c, int ind)
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

  void printArg(UP(Arg)& a, int ind)
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
    if(a->t.which() == 1)
    {
      printType(a->t.get<UP(Type)>(), ind + indentLevel);
    }
    else if(a->t.which() == 2)
    {
      printTraitType(a->t.get<UP(TraitType)>(), ind + indentLevel);
    }
  }

  void printFuncDecl(UP(FuncDecl)& fd, int ind)
  {
    indent(ind);
    cout << "Func declaration: \"" << fd->name << "\"\n";
    indent(ind);
    cout << "Return type:\n";
    printType(fd->retType, ind + indentLevel);
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

  void printFuncDef(UP(FuncDef)& fd, int ind)
  {
    indent(ind);
    cout << "Func definition:\n";
    printMember(fd->name, ind + indentLevel);
    indent(ind);
    cout << "Return type:\n";
    printType(fd->retType, ind + indentLevel);
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

  void printFuncType(UP(FuncType)& ft, int ind)
  {
    indent(ind);
    cout << "Func type:\n";
    printMember(ft->name, ind + indentLevel);
    indent(ind);
    cout << "Return type:\n";
    printType(ft->retType, ind + indentLevel);
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

  void printProcDecl(UP(ProcDecl)& pd, int ind)
  {
    indent(ind);
    cout << "Proc declaration: \"" << pd->name << "\"\n";
    indent(ind);
    cout << "Return type:\n";
    printType(pd->retType, ind + indentLevel);
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

  void printProcDef(UP(ProcDef)& pd, int ind)
  {
    indent(ind);
    cout << "Proc definition:\n";
    printMember(pd->name, ind + indentLevel);
    indent(ind);
    cout << "Return type:\n";
    printType(pd->retType, ind + indentLevel);
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

  void printProcType(UP(ProcType)& pt, int ind)
  {
    indent(ind);
    cout << "Func type:\n";
    printMember(pt->name, ind + indentLevel);
    indent(ind);
    cout << "Return type:\n";
    printType(pt->retType, ind + indentLevel);
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

  void printStructDecl(UP(StructDecl)& sd, int ind)
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

  void printVariantDecl(UP(VariantDecl)& vd, int ind)
  {
    indent(ind);
    cout << "Variant \"" << vd->name << "\"\n";
    for(auto& it : vd->types)
    {
      printType(it, ind + indentLevel);
    }
  }

  void printTraitDecl(UP(TraitDecl)& td, int ind)
  {
    indent(ind);
    cout << "Trait \"" << td->name << "\"\n";
    for(auto& it : td->members)
    {
      if(it.which() == 1)
      {
        printFuncDecl(it.get<UP(FuncDecl)>(), ind + indentLevel);
      }
      else if(it.which() == 2)
      {
        printProcDecl(it.get<UP(ProcDecl)>(), ind + indentLevel);
      }
    }
  }

  void printStructLit(UP(StructLit)& sl, int ind)
  {
    indent(ind);
    cout << "Struct/Array literal\n";
    for(auto& it : sl->vals)
    {
      printExpression(it, ind + indentLevel);
    }
  }

  void printMember(UP(Member)& m, int ind)
  {
    indent(ind);
    cout << "Member \"" << m->owner << "\":\n";
    if(m->mem)
    {
      printMember(m->mem, ind + indentLevel);
    }
  }

  void printTraitType(UP(TraitType)& tt, int ind)
  {
    indent(ind);
    cout << "TraitType \"" << tt->localName << "\", underlying:\n";
    printMember(tt->traitName, ind + indentLevel);
  }

  void printTupleType(UP(TupleType)& tt, int ind)
  {
    indent(ind);
    cout << "Tuple type, members:\n";
    for(auto& it : tt->members)
    {
      printType(it, ind + indentLevel);
    }
  }

  void printBoolLit(UP(BoolLit)& bl, int ind)
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

  void printExpr1(UP(Expr1)& e, int ind)
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

  void printExpr1RHS(UP(Expr1RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr1RHS (||)\n";
    printExpr2(e->rhs, ind + indentLevel);
  }

  void printExpr2(UP(Expr2)& e, int ind)
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

  void printExpr2RHS(UP(Expr2RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr2RHS (&&)\n";
    printExpr3(e->rhs, ind + indentLevel);
  }

  void printExpr3(UP(Expr3)& e, int ind)
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

  void printExpr3RHS(UP(Expr3RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr3RHS (|)\n";
    printExpr4(e->rhs, ind + indentLevel);
  }

  void printExpr4(UP(Expr4)& e, int ind)
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

  void printExpr4RHS(UP(Expr4RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr4RHS (^)\n";
    printExpr5(e->rhs, ind + indentLevel);
  }

  void printExpr5(UP(Expr5)& e, int ind)
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

  void printExpr5RHS(UP(Expr5RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr5RHS (&)\n";
    printExpr6(e->rhs, ind + indentLevel);
  }

  void printExpr6(UP(Expr6)& e, int ind)
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

  void printExpr6RHS(UP(Expr6RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr6RHS (" << operatorTable[e->op] << ")\n";
    printExpr7(e->rhs, ind + indentLevel);
  }

  void printExpr7(UP(Expr7)& e, int ind)
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

  void printExpr7RHS(UP(Expr7RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr7RHS (" << operatorTable[e->op] << ")\n";
    printExpr8(e->rhs, ind + indentLevel);
  }

  void printExpr8(UP(Expr8)& e, int ind)
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

  void printExpr8RHS(UP(Expr8RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr8RHS (" << operatorTable[e->op] << ")\n";
    printExpr9(e->rhs, ind + indentLevel);
  }

  void printExpr9(UP(Expr9)& e, int ind)
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

  void printExpr9RHS(UP(Expr9RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr9RHS (" << operatorTable[e->op] << ")\n";
    printExpr10(e->rhs, ind + indentLevel);
  }

  void printExpr10(UP(Expr10)& e, int ind)
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

  void printExpr10RHS(UP(Expr10RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr10RHS (" << operatorTable[e->op] << ")\n";
    printExpr11(e->rhs, ind + indentLevel);
  }

  void printExpr11(UP(Expr11)& e, int ind)
  {
    indent(ind);
    cout << "Expr11\n";
    if(e->e.which() == 1)
    {
      printExpr12(e->e.get<UP(Expr12)>(), ind + indentLevel);
    }
    else if(e->e.which() == 2)
    {
      printExpr11RHS(e->e.get<UP(Expr11RHS)>(), ind + indentLevel);
    }
  }

  void printExpr11RHS(UP(Expr11RHS)& e, int ind)
  {
    indent(ind);
    cout << "Expr11RHS (" << operatorTable[e->op] << ")\n";
    printExpr11(e->base, ind + indentLevel);
  }

  void printExpr12(UP(Expr12)& e, int ind)
  {
    indent(ind);
    switch(e->e.which())
    {
      indent(ind);
      case 1:
        cout << "Int literal: " << e->e.get<IntLit*>()->val << '\n';
        break;
      case 2:
      {
        char c = e->e.get<CharLit*>()->val;
        if(isprint(c))
          cout << "Char literal: " << c << '\n';
        else
          printf("Char literal: %#02hhx\n", c);
        break;
      }
      case 3:
        cout << "String literal: \"" << e->e.get<StrLit*>()->val << "\"\n";
        break;
      case 4:
        printf("Float literal: %.3e\n", e->e.get<FloatLit*>()->val);
        break;
      case 5:
        printBoolLit(e->e.get<UP(BoolLit)>(), ind);
        break;
      case 6:
        printExpression(e->e.get<UP(Expression)>(), ind + indentLevel);
        break;
      case 7:
        printMember(e->e.get<UP(Member)>(), ind + indentLevel);
        break;
      case 8:
        printStructLit(e->e.get<UP(StructLit)>(), ind + indentLevel);
        break;
      default:;
    }
  }
}

