#include "MiddleEnd.hpp"

ModuleScope* global = NULL;

namespace MiddleEnd
{
  vector<Subroutine*> subrsToProcess;

  void load(Parser::Module* ast)
  {
    //create global scope - no name and no parent
    global = new ModuleScope("", NULL, ast);
    TypeSystem::createBuiltinTypes();
    //create AST scopes, types, traits, subroutines
    TypeSystem::typeLookup = new TypeSystem::DeferredTypeLookup(
        TypeSystem::lookupTypeDeferred, TypeSystem::typeErrorMessage);
    TypeSystem::traitLookup = new TypeSystem::DeferredTraitLookup(
        TypeSystem::lookupTraitDeferred, TypeSystem::traitErrorMessage);
    for(auto& it : ast->decls)
    {
      visitScopedDecl(global, it);
    }
    TypeSystem::typeLookup->flush();
    TypeSystem::traitLookup->flush();
    for(auto s : subrsToProcess)
    {
      s->addStatements();
    }
  }

  void visitModule(Scope* current, Parser::Module* m)
  {
    Scope* mscope = new ModuleScope(m->name, current, m);
    //add all locally defined non-struct types in first pass:
    for(auto& it : m->decls)
    {
      visitScopedDecl(mscope, it);
    }
  }

  void visitSubroutine(Scope* current, Parser::SubroutineNT* subrNT)
  {
    //create a scope for the subroutine and its body
    SubroutineScope* ss = new SubroutineScope(current);
    for(auto param : subrNT->params)
    {
      if(param->type.is<Parser::BoundedTypeNT*>())
      {
        //create a named bounded type for the param and add to ss
        ss->addName(new TypeSystem::BoundedType(param->type.get<Parser::BoundedTypeNT*>(), ss));
      }
    }
    Subroutine* subr = new Subroutine(subrNT, ss);
    current->addName(subr);
    //add parameter variables
    for(auto param : subrNT->params)
    {
      Parser::TypeNT paramType;
      if(param->type.is<Parser::BoundedTypeNT*>())
      {
        Parser::Member tname;
        tname.names.push_back(param->type.get<Parser::BoundedTypeNT*>()->localName);
        paramType.t = &tname;
      }
      else
      {
        paramType = *param->type.get<Parser::TypeNT*>();
      }
      auto t = TypeSystem::lookupType(&paramType, ss);
      if(!t)
      {
        ERR_MSG("parameter " << param->name << " to subroutine " << subr->name << " has an unknown type");
      }
      Variable* pvar = new Variable(ss, param->name, t);
      ss->addName(pvar);
      subr->args.push_back(pvar);
    }
    //remember to visit block later (if it exists)
    if(subrNT->body)
    {
      subrsToProcess.push_back(subr);
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
      for(auto sc : sw->stmts)
      {
        visitStatement(current, sc);
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
      visitScopedDecl(sscope, it);
    }
    current->addName(new TypeSystem::StructType(sd, current, sscope));
  }

  void visitTrait(Scope* current, Parser::TraitDecl* td)
  {
    TraitScope* tscope = new TraitScope(current, td->name);
    //Create the trait
    current->addName(new TypeSystem::Trait(td, tscope));
    //trait scope can't have child scopes, so done here
  }

  void visitScopedDecl(Scope* current, Parser::ScopedDecl* sd)
  {
    if(sd->decl.is<Parser::Enum*>())
    {
      current->addName(new TypeSystem::EnumType(sd->decl.get<Parser::Enum*>(), current));
    }
    else if(sd->decl.is<Parser::Typedef*>())
    {
      current->addName(new TypeSystem::AliasType(sd->decl.get<Parser::Typedef*>(), current));
    }
    else if(sd->decl.is<Parser::StructDecl*>())
    {
      visitStruct(current, sd->decl.get<Parser::StructDecl*>());
    }
    else if(sd->decl.is<Parser::Module*>())
    {
      visitModule(current, sd->decl.get<Parser::Module*>());
    }
    else if(sd->decl.is<Parser::SubroutineNT*>())
    {
      visitSubroutine(current, sd->decl.get<Parser::SubroutineNT*>());
    }
    else if(sd->decl.is<Parser::TraitDecl*>())
    {
      visitTrait(current, sd->decl.get<Parser::TraitDecl*>());
    }
    else if(sd->decl.is<Parser::VarDecl*>())
    {
      //create the variable (ctor uses deferred lookup for type)
      //
      //if local var (scope is a block), don't create var yet
      //
      //if non-static in struct or module within struct, is struct member
      auto vd = sd->decl.get<Parser::VarDecl*>();
      bool local = dynamic_cast<BlockScope*>(current);
      StructScope* owner = dynamic_cast<StructScope*>(current);
      if(!local)
      {
        for(Scope* iter = current; iter && !owner; iter = iter->parent)
        {
          owner = dynamic_cast<StructScope*>(iter);
        }
      }
      //Semantic-check static/compose modifiers given context
      if(!owner)
      {
        if(vd->isStatic)
        {
          ERR_MSG("variable " << vd->name <<
              " declared static but not in struct");
        }
        if(vd->composed)
        {
          ERR_MSG("variable " << vd->name <<
              " declared with compose operator but not in struct");
        }
      }
      if(vd->isStatic && vd->composed)
      {
        ERR_MSG("variable " << vd->name <<
            " declared both static and with compose operator");
      }
      if(!local)
      {
        if(owner && !vd->isStatic)
        {
          owner->type->members.push_back(new Variable(current, vd, true));
          owner->type->composed.push_back(vd->composed);
        }
        else
        {
          //non-member/global
          current->addName(new Variable(current, vd));
        }
      }
    }
    else
    {
      INTERNAL_ERROR;
    }
  }
}

