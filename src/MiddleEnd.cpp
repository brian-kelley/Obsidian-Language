#include "MiddleEnd.hpp"

using namespace std;

ModuleScope* global = NULL;

namespace MiddleEnd
{
  void load(Parser::Module* ast)
  {
    //create global scope - no name and no parent
    global = new ModuleScope("", NULL, ast);
    TypeSystem::createBuiltinTypes();
    //build scope tree
    DEBUG_DO(cout << "Building scope tree and creating types...\n";);
    for(auto& it : ast->decls)
    {
      ScopeTypeLoading::visitScopedDecl(global, it);
    }
    DEBUG_DO(cout << "Resolving undefined types...\n";);
    TypeSystem::resolveAllTraits();
    TypeSystem::resolveAllTypes();
    DEBUG_DO(cout << "Building list of global/static variable declarations...\n";);
    VarLoading::visitScope(global);
    DEBUG_DO(cout << "Loading functions and procedures...\n";);
    SubroutineLoading::visitScope(global);
    DEBUG_DO(cout << "Middle end done.\n";);
  }

  namespace ScopeTypeLoading
  {
    void visitModule(Scope* current, Parser::Module* m)
    {
      Scope* mscope = new ModuleScope(m->name, current, m);
      //add all locally defined non-struct types in first pass:
      for(auto& it : m->decls)
      {
        visitScopedDecl(mscope, it);
      }
    }

    void visitBlock(Scope* current, Parser::Block* b)
    {
      BlockScope* bscope = new BlockScope(current, b);
      for(auto st : b->statements)
      {
        visitStatement(bscope, st);
      }
    }

    void visitStatement(Scope* current, Parser::StatementNT* st)
    {
      if(st->s.is<Parser::ScopedDecl*>())
      {
        visitScopedDecl(current, st->s.get<Parser::ScopedDecl*>());
      }
      else if(st->s.is<Parser::Block*>())
      {
        visitBlock(current, st->s.get<Parser::Block*>());
      }
      else if(st->s.is<Parser::For*>())
      {
        visitBlock(current, st->s.get<Parser::For*>()->body);
      }
      else if(st->s.is<Parser::While*>())
      {
        visitBlock(current, st->s.get<Parser::While*>()->body);
      }
      else if(st->s.is<Parser::If*>())
      {
        auto i = st->s.get<Parser::If*>();
        visitStatement(current, i->ifBody);
        if(i->elseBody)
        {
          visitStatement(current, i->elseBody);
        }
      }
      else if(st->s.is<Parser::Switch*>())
      {
        auto sw = st->s.get<Parser::Switch*>();
        for(auto sc : sw->cases)
        {
          visitStatement(current, sc->s);
        }
        if(sw->defaultStatement)
        {
          visitStatement(current, sw->defaultStatement);
        }
      }
    }

    void visitStruct(Scope* current, Parser::StructDecl* sd)
    {
      //must create a child scope first, and then type
      StructScope* sscope = new StructScope(sd->name, current, sd);
      //Visit the internal ScopedDecls that are types
      for(auto& it : sd->members)
      {
        auto& decl = it->sd;
        visitScopedDecl(sscope, decl);
      }
      new TypeSystem::StructType(sd, current, sscope);
    }

    void visitScopedDecl(Scope* current, Parser::ScopedDecl* sd)
    {
      if(sd->decl.is<Parser::Enum*>())
      {
        new TypeSystem::EnumType(sd->decl.get<Parser::Enum*>(), current);
      }
      else if(sd->decl.is<Parser::Typedef*>())
      {
        new TypeSystem::AliasType(sd->decl.get<Parser::Typedef*>(), current);
      }
      else if(sd->decl.is<Parser::StructDecl*>())
      {
        visitStruct(current, sd->decl.get<Parser::StructDecl*>());
      }
      else if(sd->decl.is<Parser::UnionDecl*>())
      {
        new TypeSystem::UnionType(sd->decl.get<Parser::UnionDecl*>(), current);
      }
      else if(sd->decl.is<Parser::Module*>())
      {
        visitModule(current, sd->decl.get<Parser::Module*>());
      }
      else if(sd->decl.is<Parser::FuncDef*>())
      {
        visitBlock(current, sd->decl.get<Parser::FuncDef*>()->body);
      }
      else if(sd->decl.is<Parser::ProcDef*>())
      {
        visitBlock(current, sd->decl.get<Parser::ProcDef*>()->body);
      }
      else if(sd->decl.is<Parser::TraitDecl*>())
      {
        new TypeSystem::Trait(sd->decl.get<Parser::TraitDecl*>(), current);
      }
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
          if(it->s.is<Parser::ScopedDecl*>())
          {
            auto sd = it->s.get<Parser::ScopedDecl*>();
            if(sd->decl.is<Parser::VarDecl*>())
            {
              auto vd = sd->decl.get<Parser::VarDecl*>();
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
          if(it->sd->decl.is<Parser::VarDecl*>())
          {
            auto vd = it->sd->decl.get<Parser::VarDecl*>();
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
          if(it->decl.is<Parser::VarDecl*>())
          {
            ms->vars.push_back(new Variable(s, it->decl.get<Parser::VarDecl*>()));
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
          if(it->s.is<Parser::ScopedDecl*>())
          {
            auto sd = it->s.get<Parser::ScopedDecl*>();
            visitDecl(s, sd);
          }
        }
      }
      else if(ms)
      {
        for(auto& it : ms->ast->decls)
        {
          visitDecl(s, it);
        }
      }
      else if(ss)
      {
        for(auto& it : ss->ast->members)
        {
          visitDecl(s, it->sd);
        }
      }
      for(auto child : s->children)
      {
        visitScope(child);
      }
    }

    void visitDecl(Scope* s, Parser::ScopedDecl* decl)
    {
      if(decl->decl.is<Parser::FuncDef*>())
      {
        s->subr.push_back(new Function(decl->decl.get<Parser::FuncDef*>()));
      }
      else if(decl->decl.is<Parser::ProcDef*>())
      {
        s->subr.push_back(new Procedure(decl->decl.get<Parser::ProcDef*>()));
      }
    }
  }
}

