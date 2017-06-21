#include "MiddleEndDebug.hpp"

#define INDENT 2

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
    printModuleScope(global, 0);
  }

  /******************/
  /* Scope printing */
  /******************/

  void printScope(Scope* s, int ind)
  {
    if(dynamic_cast<ModuleScope*>(s))
    {
      printModuleScope((ModuleScope*) s, ind);
    }
    else if(dynamic_cast<StructScope*>(s))
    {
      printStructScope((StructScope*) s, ind);
    }
    else if(dynamic_cast<BlockScope*>(s))
    {
      printBlockScope((BlockScope*) s, ind);
    }
  }

  void printModuleScope(ModuleScope* s, int ind)
  {
    indent(ind);
    cout << "Module scope: " << s->name << '\n';
    if(s->types.size())
    {
      indent(ind);  
      cout << "<> Types:\n";
      for(auto& it : s->types)
      {
        printType(it, ind + 1);
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

  void printStructScope(StructScope* s, int ind)
  {
    indent(ind);
    cout << "Struct scope: " << s->name << '\n';
    if(s->types.size())
    {
      indent(ind);  
      cout << "<> Types:\n";
      for(auto& it : s->types)
      {
        printType(it, ind + 1);
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

  void printBlockScope(BlockScope* s, int ind)
  {
    indent(ind);
    cout << "Block scope #" << s->index << '\n';
    if(s->types.size())
    {
      indent(ind);  
      cout << "<> Types:\n";
      for(auto& it : s->types)
      {
        printType(it, ind + 1);
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
      errAndQuit("Tried to print a null Type!\n");
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
  }

  void printStructType(StructType* t, int ind)
  {
    indent(ind);
    cout << "Struct type: " << t->name << ", members:\n";
    for(size_t i = 0; i < t->members.size(); i++)
    {
      indent(ind + 1);
      cout << t->memberNames[i] << ":\n";
      printType(t->members[i], ind + 1);
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
    cout << "Array type, " << t->dims << "-dimensional array of:\n";
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
}

