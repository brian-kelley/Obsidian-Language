#include "MiddleEnd.hpp"

using namespace std;
using namespace Parser;
using namespace TypeSystem;

ModuleScope* global = NULL;

namespace MiddleEnd
{
  void load(Module* ast)
  {
    //create global scope - no name and no parent
    global = new ModuleScope("", NULL, ast);
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
    cout << "Builing list of variable declarations...\n";
    VarLoading::visitScope(global);
    cout << "Middle end done.\n";
  }

  namespace ScopeTypeLoading
  {
    void visitModule(Scope* current, Module* m)
    {
      Scope* mscope = new ModuleScope(m->name, current, m);
      //add all locally defined non-struct types in first pass:
      for(auto& it : m->decls)
      {
        visitScopedDecl(mscope, it);
      }
    }

    void visitBlock(Scope* current, Block* b)
    {
      BlockScope* bscope = new BlockScope(current, b);
      for(auto& st : b->statements)
      {
        if(st->s.is<ScopedDecl*>())
        {
          visitScopedDecl(bscope, st->s.get<ScopedDecl*>());
        }
        else if(st->s.is<Block*>())
        {
          visitBlock(bscope, st->s.get<Block*>());
        }
      }
    }

    void visitStruct(Scope* current, StructDecl* sd)
    {
      //must create a child scope first, and then type
      StructScope* sscope = new StructScope(sd->name, current, sd);
      //Visit the internal ScopedDecls that are types
      for(auto& it : sd->members)
      {
        auto& decl = it->sd;
        visitScopedDecl(sscope, decl);
      }
      new StructType(sd, current, sscope);
    }

    void visitScopedDecl(Scope* current, ScopedDecl* sd)
    {
      if(sd->decl.is<Enum*>())
      {
        new EnumType(sd->decl.get<Enum*>(), current);
      }
      else if(sd->decl.is<Typedef*>())
      {
        new AliasType(sd->decl.get<Typedef*>(), current);
      }
      else if(sd->decl.is<StructDecl*>())
      {
        visitStruct(current, sd->decl.get<StructDecl*>());
      }
      else if(sd->decl.is<UnionDecl*>())
      {
        new UnionType(sd->decl.get<UnionDecl*>(), current);
      }
      else if(sd->decl.is<Module*>())
      {
        visitModule(current, sd->decl.get<Module*>());
      }
      else if(sd->decl.is<FuncDef*>())
      {
        visitBlock(current, sd->decl.get<FuncDef*>()->body);
      }
      else if(sd->decl.is<ProcDef*>())
      {
        visitBlock(current, sd->decl.get<ProcDef*>()->body);
      }
      else if(sd->decl.is<TraitDecl*>())
      {
        new Trait(sd->decl.get<TraitDecl*>(), current);
      }
    }
  }

  namespace SubroutineLoading
  {
    void visitScope(Scope* s)
    {
      auto bs = dynamic_cast<BlockScope*>(s);
      auto ms = dynamic_cast<ModuleScope*>(s);
      auto ss = dynamic_cast<StructScope*>(s);
      if(bs)
      {
        for(auto& it : bs->ast->statements)
        {
          if(it->s.is<ScopedDecl*>())
          {
            auto sd = it->s.get<ScopedDecl*>();
            if(sd->decl.is<FuncDef*>())
            {
              visitFuncDef(s, sd->decl.get<FuncDef*>());
            }
            else if(sd->decl.is<ProcDef*>())
            {
            }
              visitProcDef(s, sd->decl.get<ProcDef*>());
          }
        }
      }
      else if(ms)
      {
        for(auto& it : ms->ast->decls)
        {
          if(it->decl.is<FuncDef*>())
          {
            visitFuncDef(s, it->decl.get<FuncDef*>());
          }
          else if(it->decl.is<ProcDef*>())
          {
            visitProcDef(s, it->decl.get<ProcDef*>());
          }
        }
      }
      else if(ss)
      {
        for(auto& it : ss->ast->members)
        {
          if(it->sd->decl.is<FuncDef*>())
          {
            visitFuncDef(s, it->sd->decl.get<FuncDef*>());
          }
          else if(it->sd->decl.is<ProcDef*>())
          {
            visitProcDef(s, it->sd->decl.get<ProcDef*>());
          }
        }
      }
      for(auto child : children)
      {
        visitScope(child);
      }
    }

    void visitFuncDef(Scope* s, Parser::FuncDef* ast)
    {
      s->subr.push_back(new Function(s, ast));
    }

    void visitProcDef(Scope* s, Parser::ProcDef* ast)
    {
      s->subr.push_back(new Procedure(s, ast));
    }
  }

  namespace VarLoading
  {
    void visitScope(Scope* s)
    {
      //find all var decls in program, depth-first thru scope tree
      //vars are in line order (within scope)
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
          if(it->s.is<ScopedDecl*>())
          {
            auto sd = it->s.get<ScopedDecl*>();
            if(sd->decl.is<VarDecl*>())
            {
              auto vd = sd->decl.get<VarDecl*>();
              bs->vars.push_back(new Variable(s, vd));
            }
          }
        }
      }
      else if(ss)
      {
        //only process static vars here
        for(auto& it : ss->ast->members)
        {
          if(it->sd->decl.is<VarDecl*>())
          {
            auto vd = it->sd->decl.get<VarDecl*>();
            if(vd->isStatic)
            {
              ss->vars.push_back(new Variable(s, vd));
            }
          }
        }
      }
      else if(ms)
      {
        for(auto& it : ms->ast->decls)
        {
          if(it->decl.is<VarDecl*>())
          {
            ms->vars.push_back(new Variable(s, it->decl.get<VarDecl*>()));
          }
        }
      }
      //visit all child scopes
      for(auto child : s->children)
      {
        visitScope(child);
      }
    }
  }
}

