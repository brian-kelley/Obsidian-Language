#include "MiddleEndDebug.hpp" 
#define INDENT 2

using namespace TypeSystem;

namespace MiddleEndDebug
{
  static void indent(int level)
  {
    for(int i = 0; i < INDENT * level; i++)
    {
      cout << ' ';
    }
  }

  void printTypeTree()
  {
    cout << ">>> Global (root) scope:\n";
    printScope(global, 0);
  }

  /******************/
  /* Scope printing */
  /******************/

  void printScope(Scope* s, int ind)
  {
    //First, handle features of base scopes (permanent scoped decls)
    if(dynamic_cast<ModuleScope*>(s))
    {
      ModuleScope* m = (ModuleScope*) s;
      indent(ind);
      if(m->name.length())
        cout << "(*) Module scope: " << m->name << '\n';
    }
    else if(dynamic_cast<StructScope*>(s))
    {
      StructScope* ss = (StructScope*) s;
      indent(ind);
      cout << "(*) Struct scope: " << ss->name << '\n';
    }
    else if(dynamic_cast<BlockScope*>(s))
    {
      BlockScope* b = (BlockScope*) s;
      indent(ind);
      cout << "(*) Block scope #" << b->index << '\n';
    }
    printScopeBody(s, ind);
  }

  void printScopeBody(Scope* s, int ind)
  {
    if(s->types.size())
    {
      indent(ind);  
      cout << "<> Types:\n";
      for(auto& it : s->types)
      {
        printType(it, ind + 1);
      }
    }
    /*
    if(s->traits.size())
    {
      indent(ind);
      cout << "<> Traits:\n";
      for(auto& it : s->traits)
      {
        printTrait(it, ind + 1);
      }
    }
    */
    if(s->vars.size())
    {
      indent(ind);
      cout << "<> Variables:\n";
      for(auto it : s->vars)
      {
        printVariable(it, ind + 1);
      }
    }
    if(s->children.size())
    {
      indent(ind);
      cout << "<> Child scopes:\n";
      for(auto& it : s->children)
      {
        printScope(it, ind + 1);
      }
    }
  }

  /*****************/
  /* Type printing */
  /*****************/

  void printType(Type* t, int ind)
  {
    if(!t)
    {
      indent(ind);
      cout << "WARNING: null Type!\n";
    }
    else if(dynamic_cast<StructType*>(t))
    {
      printStructType((StructType*) t, ind);
    }
    else if(dynamic_cast<UnionType*>(t))
    {
      printUnionType((UnionType*) t, ind);
    }
    else if(dynamic_cast<AliasType*>(t))
    {
      printAliasType((AliasType*) t, ind);
    }
    else if(dynamic_cast<EnumType*>(t))
    {
      printEnumType((EnumType*) t, ind);
    }
    else if(dynamic_cast<ArrayType*>(t))
    {
      printArrayType((ArrayType*) t, ind);
    }
    else if(dynamic_cast<TupleType*>(t))
    {
      printTupleType((TupleType*) t, ind);
    }
    else if(dynamic_cast<IntegerType*>(t))
    {
      indent(ind);
      IntegerType* it = (IntegerType*) t;
      cout << "Integer type: " << it->name << ", ";
      if(!it->isSigned)
      {
        cout << "un";
      }
      cout << "signed " << it->size * 8 << "-bit\n";
    }
    else if(dynamic_cast<FloatType*>(t))
    {
      indent(ind);
      FloatType* ft = (FloatType*) t;
      cout << "Float type: " << ft->name << ", " << ft->size * 8 << "-bit\n";
    }
    else if(dynamic_cast<BoolType*>(t))
    {
      indent(ind);
      cout << "bool\n";
    }
    else if(dynamic_cast<VoidType*>(t))
    {
      indent(ind);
      cout << "void\n";
    }
    /*
    else if(dynamic_cast<FuncType*>(t))
    {
      printFuncType((FuncType*) t, ind);
    }
    else if(dynamic_cast<ProcType*>(t))
    {
      printProcType((ProcType*) t, ind);
    }
    else if(dynamic_cast<TType*>(t))
    {
      indent(ind);
      cout << "T (in trait)\n";
    }
    */
    else
    {
      indent(ind);
      cout << "ERROR: unknown Type subclass!\n";
    }
  }

  void printStructType(StructType* t, int ind)
  {
    indent(ind);
    cout << "Struct type: " << t->name << '\n';
    /*
    if(t->traits.size())
    {
      indent(ind);
      cout << "Traits:\n";
      for(size_t i = 0; i < t->traits.size(); i++)
      {
        //just print trait names for brevity
        indent(ind + 1);
        cout << t->traits[i]->name << 'n';
      }
    }
    */
    indent(ind);
    cout << "Members:\n";
    for(size_t i = 0; i < t->members.size(); i++)
    {
      indent(ind + 1);
      cout << t->memberNames[i] << ":\n";
      printType(t->members[i], ind + 2);
    }
  }

  void printUnionType(UnionType* t, int ind)
  {
    indent(ind);
    cout << "Union type: " << t->name << ", options:\n";
    for(auto& it : t->options)
    {
      printType(it, ind + 1);
    }
  }

  void printAliasType(AliasType* t, int ind)
  {
    indent(ind);
    cout << "Alias type: " << t->name << ", underlying type:\n";
    printType(t->actual, ind + 1);
  }

  void printEnumType(EnumType* t, int ind)
  {
    indent(ind);
    cout << "Enum type: " << t->name << ", values:\n";
    for(auto& it : t->values)
    {
      indent(ind + 1);
      cout << it.first << " = " << it.second << '\n';
    }
  }

  void printArrayType(ArrayType* t, int ind)
  {
    indent(ind);
    cout << t->dims << "-dimensional array of:\n";
    printType(t->elem, ind + 1);
  }

  void printTupleType(TupleType* t, int ind)
  {
    indent(ind);
    cout << "Tuple type, members:\n";
    for(auto& it : t->members)
    {
      printType(it, ind + 1);
    }
  }

/*
  void printFuncType(FuncType* t, int ind)
  {
    indent(ind);
    cout << "Function type:\n";
    indent(ind);
    cout << "Return type:\n";
    printType(t->retType, ind + 1);
    for(size_t i = 0; i < t->argTypes.size(); i++)
    {
      indent(ind);
      cout << "Arg " << i << ":\n";
      printType(t->argTypes[i], ind + 1);
    }
  }

  void printProcType(ProcType* t, int ind)
  {
    indent(ind);
    cout << "Procedure type:\n";
    indent(ind);
    if(t->nonterm)
    {
      cout << "Non-terminating.\n";
    }
    else
    {
      cout << "Terminating\n";
    }
    indent(ind);
    cout << "Return type:\n";
    printType(t->retType, ind + 1);
    for(size_t i = 0; i < t->argTypes.size(); i++)
    {
      indent(ind);
      cout << "Arg " << i << ":\n";
      printType(t->argTypes[i], ind + 1);
    }
  }
  */

  /*
  void printTrait(TypeSystem::Trait* t, int ind)
  {
    indent(ind);
    cout << "Trait " << t->name << '\n';
    if(t->funcs.size())
    {
      for(auto f : t->funcs)
      {
        indent(ind + 1);
        cout << "Function \"" << f.name << "\":\n";
        printFuncType(f.type, ind + 2);
      }
    }
    if(t->procs.size())
    {
      for(auto p : t->procs)
      {
        indent(ind + 1);
        cout << "Procedure \"" << p.name << "\":\n";
        printProcType(p.type, ind + 2);
      }
    }
  }
  */

  void printVariable(Variable* v, int ind)
  {
    indent(ind);
    cout << "Variable \"" << v->name << "\":\n";
    indent(ind);
    cout << "Type:\n";
    printType(v->type, ind + 1);
  }

  void printSubroutines(Scope* s, int ind)
  {
    indent(ind);
    if(s == global)
    {
      cout << s->subr.size() << " subroutine(s) in global scope:\n";
    }
    else
    {
      cout << s->subr.size() << " subroutine(s) in scope " << s->getLocalName() << ":\n";
    }
    for(auto it : s->subr)
    {
      auto f = dynamic_cast<Function*>(it);
      auto p = dynamic_cast<Procedure*>(it);
      if(f)
      {
        printFunc(f, ind + 1);
      }
      else
      {
        printProc(p, ind + 1);
      }
    }
    for(auto c : s->children)
    {
      printSubroutines(c, ind + 1);
    }
  }

  void printFunc(Function* f, int ind)
  {
    indent(ind);
    cout << "Function " << f->name << ":\n";
    printStatement(f->body, ind + 1);
  }

  void printProc(Procedure* p, int ind)
  {
    indent(ind);
    cout << "Procedure " << p->name << ":\n";
    printStatement(p->body, ind + 1);
  }

  void printStatement(Statement* s, int ind)
  {
    auto b = dynamic_cast<Block*>(s);
    auto a = dynamic_cast<Assign*>(s);
    auto c = dynamic_cast<CallStmt*>(s);
    auto f = dynamic_cast<For*>(s);
    auto w = dynamic_cast<While*>(s);
    auto i = dynamic_cast<If*>(s);
    auto ie = dynamic_cast<IfElse*>(s);
    auto r = dynamic_cast<Return*>(s);
    auto br = dynamic_cast<Break*>(s);
    auto co = dynamic_cast<Continue*>(s);
    auto p = dynamic_cast<Print*>(s);
    auto as = dynamic_cast<Assertion*>(s);
    if(b)
    {
      printBlock(b, ind);
    }
    else if(a)
    {
      indent(ind);
      cout << "Assignment of expr " << a->rvalue << " to expr " << a->lvalue << '\n';
    }
    else if(c)
    {
      indent(ind);
      cout << "Call to procedure " << c->called->name << " with args:\n";
      for(auto arg : c->args)
      {
        indent(ind + 1);
        cout << arg << '\n';
      }
    }
    else if(f)
    {
      indent(ind);
      cout << "For loop\n";
      indent(ind);
      cout << "Initializer:\n";
      printStatement(f->init, ind + 1);
      cout << "Condition expr\n";
      indent(ind + 1);
      cout << f->condition;
      indent(ind);
      cout << "Increment:\n";
      printStatement(f->increment, ind + 1);
      cout << "Body:\n";
      printBlock(f->loopBlock, ind + 1);
    }
    else if(w)
    {
      indent(ind);
      cout << "While loop\n";
      indent(ind);
      cout << "Condition: " << w->condition << '\n';
      indent(ind);
      cout << "Body:\n";
      printBlock(w->loopBlock, ind + 1);
    }
    else if(i)
    {
      indent(ind);
      cout << "If: condition " << i->condition << "\n";
      indent(ind);
      cout << "Body:\n";
      printStatement(i->body, ind + 1);
    }
    else if(ie)
    {
      indent(ind);
      cout << "If-else: condition " << ie->condition << "\n";
      indent(ind);
      cout << "True body:\n";
      printStatement(ie->trueBody, ind + 1);
      cout << "False body:\n";
      printStatement(ie->falseBody, ind + 1);
    }
    else if(r)
    {
      indent(ind);
      cout << "Return ";
      if(r->value)
      {
        cout << "expr " << r->value << ' ';
      }
      else
      {
        cout << "void ";
      }
      cout << "from subroutine " << r->from << '\n';
    }
    else if(br)
    {
      indent(ind);
      cout << "Break from ";
      if(br->loop->is<For*>())
      {
        cout << "for loop " << br->loop->get<For*>() << '\n';
      }
      else
      {
        cout << "while loop " << br->loop->get<While*>() << '\n';
      }
    }
    else if(co)
    {
      indent(ind);
      cout << "Countine: return to start of ";
      if(co->loop->is<For*>())
      {
        cout << "for loop " << co->loop->get<For*>() << '\n';
      }
      else
      {
        cout << "while loop " << co->loop->get<While*>() << '\n';
      }
    }
    else if(p)
    {
      indent(ind);
      cout << "Print of expressions:\n";
      for(auto e : p->exprs)
      {
        indent(ind + 1);
        cout << e << '\n';
      }
    }
    else if(as)
    {
      indent(ind);
      cout << "Assertion that expression " << as->asserted << " is true\n";
    }
  }

  void printBlock(Block* b, int ind)
  {
    indent(ind);
    cout << "Block:\n";
    for(auto stmt : b->stmts)
    {
      printStatement(stmt, ind + 1);
    }
  }
}

