#include "MiddleEnd.hpp"

using namespace std;
using namespace Parser;
using namespace TypeSystem;

ModuleScope* global = NULL;

namespace MiddleEnd
{
  void load(AP(Module)& ast)
  {
    //create global scope - no name and no parent
    global = new ModuleScope("", NULL, ast.get());
    TypeSystem::createBuiltinTypes();
    //build scope tree
    cout << "Building scope tree and creating types...\n";
    for(auto& it : ast->decls)
    {
      ScopeTypeLoading::visitScopedDecl(global, it);
    }
    cout << "Resolving undefined types...\n";
    resolveAllTraits();
    resolveAllTypes();
    cout << "Middle end done.\n";
  }

  namespace ScopeTypeLoading
  {
    void visitModule(Scope* current, AP(Module)& m)
    {
      Scope* mscope = new ModuleScope(m->name, current, m.get());
      //add all locally defined non-struct types in first pass:
      for(auto& it : m->decls)
      {
        visitScopedDecl(mscope, it);
      }
    }

    void visitBlock(Scope* current, AP(Block)& b)
    {
      Scope* bscope = new BlockScope(current, b.get());
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
      StructScope* sscope = new StructScope(sd->name, current, sd.get());
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
      else if(sd->decl.is<AP(TraitDecl)>())
      {
        new Trait(sd->decl.get<AP(TraitDecl)>().get(), current);
      }
    }
  }

  namespace VarLoading
  {
    void visitScope(Scope* s)
    {
      //find all var decls (in line order)
      //scan through all statements and/or scoped decls in scope
      //Note: BlockScope can have Statements which are ScopedDecls which are VarDecls
      //ModuleScope and StructScope can only have ScopedDecls which are VarDecls
      //Will search through the stored AST node corresponding to Scope
      auto bs = dynamic_cast<BlockScope*>(s);
      auto ss = dynamic_cast<StructScope*>(s);
      auto ms = dynamic_cast<ModuleScope*>(s);
      if(bs)
      {
        for(auto& it : bs->ast->statements)
        {
          if(it->s.is<AP(VarDecl)>())
          {
            bs->vars.push_back(new Variable(s, it->s.get<AP(VarDecl)>().get()));
          }
        }
      }
      else if(ss)
      {
        //only process static vars here
        for(auto& it : ss->ast->members)
        {
          if(it->sd->decl.is<AP(VarDecl)>())
          {
            ss->vars.push_back(new Variable(s, it->sd->decl.get<AP(VarDecl)>().get()));
          }
        }
      }
      else if(ms)
      {
        for(auto& it : ms->ast->decls)
        {
          if(it->decl.is<AP(VarDecl)>())
          {
            ms->vars.push_back(new Variable(s, it->decl.get<AP(VarDecl)>().get()));
          }
        }
      }
    }

    void visitVarDecl(Scope* s, AP(Parser::VarDecl)& vd)
    {
    }
  }
}



