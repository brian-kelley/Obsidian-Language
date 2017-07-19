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
      cout << "Module scope: " << m->name << '\n';
    }
    else if(dynamic_cast<StructScope*>(s))
    {
      StructScope* ss = (StructScope*) s;
      indent(ind);
      cout << "Struct scope: " << ss->name << '\n';
    }
    else if(dynamic_cast<BlockScope*>(s))
    {
      BlockScope* b = (BlockScope*) s;
      indent(ind);
      cout << "Block scope #" << b->index << '\n';
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
    if(s->traits.size())
    {
      indent(ind);
      cout << "<> Traits:\n";
      for(auto& it : s->traits)
      {
        printTrait(it, ind + 1);
      }
    }
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
      cout << "Warning: null Type!\n";
    }
    if(dynamic_cast<StructType*>(t))
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
    else if(dynamic_cast<StringType*>(t))
    {
      indent(ind);
      cout << "string\n";
    }
    else if(dynamic_cast<BoolType*>(t))
    {
      indent(ind);
      cout << "bool\n";
    }
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
  }

  void printStructType(StructType* t, int ind)
  {
    indent(ind);
    cout << "Struct type: " << t->name << '\n';
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

  void printFuncType(FuncType* t, int ind)
  {
    indent(ind);
    cout << "Function type:\n";
    indent(ind);
    cout << "Return type:\n";
    printType(t->retType, ind + 1);
    indent(ind);
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
    indent(ind);
    for(size_t i = 0; i < t->argTypes.size(); i++)
    {
      indent(ind);
      cout << "Arg " << i << ":\n";
      printType(t->argTypes[i], ind + 1);
    }
  }

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

  void printVariable(Variable* v, int ind)
  {
    indent(ind);
    cout << "Variable \"" << v->name << "\":\n";
    indent(ind);
    cout << "Type:\n";
    printType(v->type, ind + 1);
  }
}

