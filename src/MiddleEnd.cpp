#include "MiddleEnd.hpp"

using namespace std;
using namespace Parser;

ModuleScope* global;

namespace MiddleEnd
{
  void load(AP(Module)& ast)
  {
    global = new ModuleScope("", nullptr);
    Type::createBuiltinTypes();
    //build scope tree
    TypeLoading::visitModule(NULL, ast);
    TypeLoading::resolveAll();
  }

  namespace TypeLoading
  {
    void visitModule(Scope* current, AP(Module)& m)
    {
      Scope* mscope = new ModuleScope(m->name, current);
      //add all locally defined non-struct types in first pass:
      for(auto& it : m->decls)
      {
        visitScopedDecl(mscope, it);
      }
    }

    void visitBlock(Scope* current, AP(Block)& b)
    {
      Scope* bscope = new BlockScope(current);
      for(auto& st : b->statements)
      {
        if(st->s.is<AP(ScopedDecl)>())
        {
          visitScopedDecl(bscope, st->s.get<AP(ScopedDecl)>());
        }
      }
    }

    void visitStruct(Scope* current, AP(StructDecl)& sd)
    {
      //must create a child scope first, and then type
      Scope* sscope = new StructScope(sd->name, current);
      //Visit the internal ScopedDecls that are types
      for(auto& it : sd->members)
      {
        auto& decl = it->sd;
        visitScopedDecl(sscope, decl);
      }
      new StructType(sd.get(), current, sscope);
    }

    void visitScopedDecl(Scope* current, AP(ScopedDecl)& sd)
    {
      if(!sd->decl.is<AP(Typedef)>() &&
          !sd->decl.is<AP(Enum)>() &&
          !sd->decl.is<AP(UnionDecl)>() &&
          !sd->decl.is<AP(StructDecl)>())
      {
        //not a type creation, nothing to be done now
        return;
      }
      if(sd->decl.is<AP(Enum)>())
      {
        EnumType* et = new EnumType(sd->decl.get<AP(Enum)>().get(), current);
      }
      else if(sd->decl.is<AP(Typedef)>())
      {
        AliasType* at = new AliasType(sd->decl.get<AP(Typedef)>().get(), current);
      }
      else if(sd->decl.is<AP(StructDecl)>())
      {
        visitStruct(current, sd->decl.get<AP(StructDecl)>());
      }
      else if(sd->decl.is<AP(UnionDecl)>())
      {
        UnionType* ud = new UnionType(sd->decl.get<AP(UnionDecl)>().get(), current);
      }
    }

    void resolveAll()
    {
      for(auto& unres : Type::unresolvedTypes)
      {
        {
          StructType* t = dynamic_cast<StructType*>(unres);
          if(t)
          {
            resolveStruct(t);
            continue;
          }
        }
        {
          UnionType* t = dynamic_cast<UnionType*>(unres);
          if(t)
          {
            resolveStruct(t);
            continue;
          }
        }
        {
          TupleType* t = dynamic_cast<TupleType*>(unres):
          if(t)
          {
            resolveStruct(t);
            continue;
          }
        }
        {
          AliasType* t = dynamic_cast<AliasType*>(unres):
          if(t)
          {
            resolveAlias(t);
            continue;
          }
        }
        {
          ArrayType* t = dynamic_cast<ArrayType*>(unres):
          if(t)
          {
            resolveArray(t);
            continue;
          }
        }
      }
    }
  }
}

