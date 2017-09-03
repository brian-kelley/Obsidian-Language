#include "Scope.hpp"
#include "TypeSystem.hpp"
#include "Variable.hpp"
#include "Subroutine.hpp"

int BlockScope::nextBlockIndex = 0;

/*******************************
*   Scope & subclasses impl    *
*******************************/

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

void Scope::findSubImpl(vector<string>& names, vector<Scope*>& matches)
{
  if(names.size() == 0)
  {
    //search this scope and then all parents
    for(Scope* iter = this; iter; iter = iter->parent)
      matches.push_back(iter);
    return;
  }
  //does this contain a scope chain with path given by names?
  Scope* next = this;
  bool found = false;
  for(auto n : names)
  {
    for(auto child : next->children)
    {
      if(child->getLocalName() == n)
      {
        found = true;
        next = child;
        break;
      }
    }
    if(!found)
      break;
  }
  //if out of the loop with found == true, next is the requested child scope
  if(found)
    matches.push_back(next);
  if(!parent)
  {
    //no more scopes to seach, so done
    return;
  }
  else
  {
    //append any matches found in parent scopes
    parent->findSubImpl(names, matches);
  }
}

TypeSystem::Type* Scope::findType(Parser::Member* mem)
{
  //check for primitives
  if(mem->scopes.size() == 0)
  {
    auto prim = TypeSystem::primNames.find(mem->ident);
    if(prim != TypeSystem::primNames.end())
    {
      return prim->second;
    }
  }
  auto search = findSub(mem->scopes);
  for(auto scope : search)
  {
    for(TypeSystem::Type* t : scope->types)
    {
      //check for named type
      auto at = dynamic_cast<TypeSystem::AliasType*>(t);
      auto st = dynamic_cast<TypeSystem::StructType*>(t);
      auto ut = dynamic_cast<TypeSystem::UnionType*>(t);
      auto et = dynamic_cast<TypeSystem::EnumType*>(t);
      if((at && at->name == mem->ident) ||
         (st && st->name == mem->ident) ||
         (ut && ut->name == mem->ident) ||
         (et && et->name == mem->ident))
      {
        return t;
      }
      INTERNAL_ERROR;
    }
  }
  return nullptr;
}

Variable* Scope::findVariable(Parser::Member* mem)
{
  auto search = findSub(mem->scopes);
  for(auto scope : search)
  {
    for(Variable* v : scope->vars)
    {
      if(v->name == mem->ident)
      {
        return v;
      }
    }
  }
  return nullptr;
}

/*
TypeSystem::Trait* Scope::findTrait(Parser::Member* mem)
{
  auto search = findSub(mem->scopes);
  for(auto scope : search)
  {
    for(TypeSystem::Trait* t : scope->traits)
    {
      if(t->name == mem->ident)
      {
        return t;
      }
    }
  }
  return nullptr;
}
*/

Subroutine* Scope::findSubroutine(Parser::Member* mem)
{
  auto search = findSub(mem->scopes);
  for(auto scope : search)
  {
    for(Subroutine* s : scope->subr)
    {
      if(s->name == mem->ident)
      {
        return s;
      }
    }
  }
  return nullptr;
}

vector<Scope*> Scope::findSub(vector<string>& names)
{
  vector<Scope*> matches;
  findSubImpl(names, matches);
  return matches;
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

/* BlockScope */

BlockScope::BlockScope(Scope* par, Parser::Block* astIn) : Scope(par), ast(astIn), index(nextBlockIndex++)
{
  ast->bs = this;
}

BlockScope::BlockScope(Scope* par) : Scope(par), ast(nullptr), index(nextBlockIndex++) {}

string BlockScope::getLocalName()
{
  //TODO: prevent all other identifiers from having a name which could be confused as a block name
  return string("_B") + to_string(index);
}

