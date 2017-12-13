#include "Scope.hpp"
#include "TypeSystem.hpp"
#include "Variable.hpp"
#include "Subroutine.hpp"

int BlockScope::nextBlockIndex = 0;

/*******************************
*   Scope & subclasses impl    *
*******************************/

#define ADD_NAME(T, tname, tenum) \
  void Scope::addName(T* item) \
  { \
    if(names.find(m->name) != names.end()) \
      ERR_MSG(tname << ' ' << item->name << " causes scope name conflict"); \
    shadowCheck(m->name, string(tname) == "variable"); \
    names[item->name] = Name(m, Name::##tenum); \
  }

ADD_NAME(ModuleScope,            "module",      Name::MODULE);
ADD_NAME(TypeSystem::StructType, "struct",      Name::STRUCT);
ADD_NAME(TypeSystem::EnumType,   "enum",        Name::ENUM);
ADD_NAME(TypeSystem::AliasType,  "typedef",     Name::TYPEDEF);
ADD_NAME(TypeSystem::BoundedType,"bounded type",Name::BOUNDED_TYPE);
ADD_NAME(TypeSystem::Trait,      "trait",       Name::TRAIT);
ADD_NAME(Subroutine,             "subroutine",  Name::SUBROUTINE);
ADD_NAME(Variable,               "variable",    Name::VARIABLE);

Scope::Scope(Scope* parentIn)
{
  parent = parentIn;
  if(parent)
  {
    parent->children.push_back(this);
  }
}

string Scope::getFullPath()
{
  if(parent)
    return parent->getFullPath() + '_' + getLocalName();
  else
    return getLocalName();
}

Name Scope::lookup(string name)
{
  auto it = names.find(name);
  if(it == names.end())
    return Name();
  return it->second;
}

Name Scope::findName(Parser::Member* mem)
{
  //scope is the scope that actually contains name mem->tail
  Scope* scope = this;
  for(size_t i = 0; i < mem->head.size(); i++)
  {
    Name it = scope->lookup(mem->head[i]->name);
    if(it.item)
    {
      //make sure that it is actually a scope of some kind
      //(MODULE and STRUCT are the only named scopes for this purpose)
      if(name.kind == MODULE)
      {
        //module is already scope
        scope = (Scope*) name.item;
        continue;
      }
      else if(name.kind == STRUCT)
      {
        scope = ((TypeSystem::StructType*) name.item)->structScope;
        continue;
      }
    }
    scope = nullptr;
    break;
  }
  if(scope)
  {
    Name it = scope->lookup(mem->tail->name);
    if(it.item != NULL)
      return it;
  }
  if(parent)
    return parent->findName(mem);
  //failure
  return Name();
}

void Scope::shadowCheck(string name, bool isVar)
{
  for(Scope* iter = this; iter; iter = iter->parent)
  {
    Name n = iter->lookup(name);
    if(n.item)
    {
      //found decl with same name: emit shadow error if this is not
      //a variable shadowing another variable
      if(!(isVar && n.kind == Name::VARIABLE))
      {
        ERR_MSG("name " << name << " shadows a previous declaration");
      }
    }
  }
}

/* ModuleScope */

ModuleScope::ModuleScope(string nameIn, Scope* par, Parser::Module* astIn) : Scope(par)
{
  name = nameIn;
  ast = astIn;
}

string ModuleScope::getLocalName()
{
  return name;
}

/* StructScope */

StructScope::StructScope(string nameIn, Scope* par, Parser::StructDecl* astIn) : Scope(par), ast(astIn), name(nameIn) {}

string StructScope::getLocalName()
{
  return name;
}

/* SubroutineScope */

string SubroutineScope::getLocalName()
{
  return subr->name;
}

/* BlockScope */

BlockScope::BlockScope(Scope* par, Parser::Block* astIn) : Scope(par), ast(astIn), index(nextBlockIndex++)
{
  ast->bs = this;
}

BlockScope::BlockScope(Scope* par) : Scope(par), ast(nullptr), index(nextBlockIndex++) {}

string BlockScope::getLocalName()
{
  //note: Onyx identifiers can't begin with underscore, so if it ever
  //matters this local name can't conflict with any other scope name
  return string("_B") + to_string(index);
}

/* TraitScope */

TraitScope::TraitScope(Scope* par, string n) : Scope(par), ttype(nullptr), name(n) {}

string TraitScope::getLocalName()
{
  return name;
}

