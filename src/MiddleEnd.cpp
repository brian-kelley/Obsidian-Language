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
    cout << "Building scope tree...\n";
    TypeLoading::visitModule(NULL, ast);
    cout << "Resolving undefined types...\n";
    TypeLoading::resolveAll();
    cout << "Middle end done.\n";
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
      StructScope* sscope = new StructScope(sd->name, current);
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
        new EnumType(sd->decl.get<AP(Enum)>().get(), current);
      }
      else if(sd->decl.is<AP(Typedef)>())
      {
        new AliasType(sd->decl.get<AP(Typedef)>().get(), current);
      }
      else if(sd->decl.is<AP(StructDecl)>())
      {
        visitStruct(current, sd->decl.get<AP(StructDecl)>());
      }
      else if(sd->decl.is<AP(UnionDecl)>())
      {
        new UnionType(sd->decl.get<AP(UnionDecl)>().get(), current);
      }
    }

    void resolveAll()
    {
      for(auto& unres : Type::unresolvedTypes)
      {
        unres->resolve();
      }
    }
  }
}

