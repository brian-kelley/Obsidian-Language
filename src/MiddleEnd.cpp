#include "MiddleEnd.hpp"

using namespace std;
using namespace Parser;

ModuleScope* global = NULL;

namespace MiddleEnd
{
  void load(AP(Module)& ast)
  {
    //create global scope - no name and no parent
    global = new ModuleScope("", NULL);
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
        else if(st->s.is<AP(Block)>())
        {
          visitBlock(bscope, st->s.get<AP(Block)>());
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
      if(sd->decl.is<AP(Enum)>())
      {
        new EnumType(sd->decl.get<AP(Enum)>().get(), current);
      }
      else if(sd->decl.is<AP(Typedef)>())
      {
        cout << "Adding an alias type. Typedef was: \n";
        AstPrinter::printTypedef(sd->decl.get<AP(Typedef)>().get(), 0);
        cout << "\n\n\n";
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
      else if(sd->decl.is<AP(Module)>())
      {
        visitModule(current, sd->decl.get<AP(Module)>());
      }
      else if(sd->decl.is<AP(FuncDef)>())
      {
        visitBlock(current, sd->decl.get<AP(FuncDef)>()->body);
      }
      else if(sd->decl.is<AP(ProcDef)>())
      {
        visitBlock(current, sd->decl.get<AP(ProcDef)>()->body);
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

