#include "Scope.hpp"
#include "TypeSystem.hpp"
#include "Variable.hpp"
#include "Subroutine.hpp"

int BlockScope::nextBlockIndex = 0;

/*******************************
*   Scope & subclasses impl    *
*******************************/

template<> void Scope::addName(Scope* s)
{
  names[s->getLocalName()] = Name(s, Name::SCOPE);
}

template<> void Scope::addName(TypeSystem::StructType* s)
{
  names[s->getName()] = Name(s, Name::STRUCT);
}

template<> void Scope::addName(TypeSystem::UnionType* u)
{
  names[u->getName()] = Name(u, Name::UNION);
}

template<> void Scope::addName(TypeSystem::EnumType* e)
{
  names[e->getName()] = Name(e, Name::ENUM);
}

template<> void Scope::addName(TypeSystem::AliasType* a)
{
  names[a->getName()] = Name(a, Name::TYPEDEF);
}

template<> void Scope::addName(Variable* v)
{
  names[s->getName()] = Name(s, Name::VARIABLE);
}

template<> void Scope::addName(Subroutine* s)
{
  names[s->name] = Name(s, Name::SUBROUTINE);
}

/* template<> void Scope::addName(TypeSystem::Trait* t)
{
  names[t->name] = Name(s, Name::SUBROUTINE);
} */

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

bool Scope::lookup(vector<string> names, Name& found, vector<string>& remain)
{
  //Rules for name lookup:
  //  -find names in this scope or any parent, up to global
  //  -In a scope, no name can shadow any names in scopes above
  //    -so never ambiguous
  for(Scope* start = this; start; start = start->parent)
  {
    Scope* search = start;
    for(size_t i = 0; i < names.size(); i++)
    {
      auto it = search->names.find(names[i]);
      if(it != search.end())
      {
        //found the next name in search
        if(i == names.size() - 1)
        {
          //found the last name is the correct one
          found = it->second;
          //no remaining names
          remain.resize(0);
          return true;
        }
        else if(it->second.type != Name::SCOPE)
        {
          //found the first non-scope name, return it
          found = it->second;
          remain.clear();
          for(size_t j = i + 1; j < names.size(); j++)
          {
            remain.push_back(names[j]);
          }
          return true;
        }
        else
        {
          //continue searching from scope it->second
          search = (Scope*) it->second.item;
        }
      }
      else
      {
        //name not found, look from a different start scope
        break;
      }
    }
  }
  found = false;
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
  //note: Onyx identifiers can't begin with underscore, so if it ever
  //matters this local name can't conflict with any other scope name
  return string("_B") + to_string(index);
}

/* TraitScope */

TraitScope::TraitScope(Scope* par, Parser::TraitDecl* astIn) : Scope(par), ast(astIn) {}

string TraitScope::getLocalName()
{
  return ast->name;
}

