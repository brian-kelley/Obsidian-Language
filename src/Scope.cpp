#include "Scope.hpp"
#include "TypeSystem.hpp"
#include "Variable.hpp"
#include "Subroutine.hpp"

using namespace TypeSystem;

Scope* global;

bool Name::inScope(Scope* s)
{
  //see if scope is same as, or child of, s
  for(Scope* iter = scope; iter = iter->parent; iter++)
  {
    if(iter == s)
    {
      return true;
    }
  }
  return false;
}

/*******************************
*   Scope & subclasses impl    *
*******************************/

Scope::Scope(Scope* p, Module* m) : parent(p), node(m) {}
Scope::Scope(Scope* p, Struct* s) : parent(p), node(s) {}
Scope::Scope(Scope* p, Subroutine* s) : parent(p), node(s) {}
Scope::Scope(Scope* p, Block* b) : parent(p), node(b) {}
Scope::Scope(Scope* p, Enum* e) : parent(p), node(e) {}

void Scope::addName(Name n)
{
  Name prev = findName(n.name);
  if(prev.item)
  {
    errMsgLoc("name " << n.name << " redefined (previous declaration on line " << prev.node->line << ", col " << prev.node->col << ")");
  }
  names[n.name] = n;
}

#define IMPL_ADD_NAME(type) \
void Scope::addName(type* item) \
{ \
  addName(Name(item, item->scope)); \
}

IMPL_ADD_NAME(Variable)
IMPL_ADD_NAME(Module)
IMPL_ADD_NAME(StructType)
IMPL_ADD_NAME(Subroutine)
IMPL_ADD_NAME(Alias)
IMPL_ADD_NAME(ExternalSubroutine)
IMPL_ADD_NAME(Enum)
IMPL_ADD_NAME(EnumConstant)

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
  for(size_t i = 0; i < mem->names.size(); i++)
  {
    Name it = scope->lookup(mem->names[i]);
    if(it.item && i == mem->names.size() - 1)
    {
      return it;
    }
    else if(it.item)
    {
      //make sure that it is actually a scope of some kind
      //(MODULE and STRUCT are the only named scopes for this purpose)
      if(it.kind == Name::MODULE)
      {
        //module is already scope
        scope = (Scope*) it.item;
        continue;
      }
      else if(it.kind == Name::STRUCT)
      {
        scope = ((TypeSystem::StructType*) it.item)->structScope;
        continue;
      }
    }
    else
    {
      scope = nullptr;
      break;
    }
  }
  //try search again in parent scope
  if(parent)
    return parent->findName(mem);
  //failure
  return Name();
}

Name Scope::findName(string name)
{
  Parser::Member m;
  m.names.push_back(name);
  return findName(&m);
}

StructType* Scope::getStructContext()
{
  //walk up scope tree, looking for a Struct scope before
  //reaching a static subroutine or global scope
  for(Scope* iter = this; iter; iter = iter->parent)
  {
    if(iter->node.is<Struct*>())
    {
      return iter->node.get<Struct*>();
    }
    if(iter->node.is<Subroutine*>())
    {
      auto subrType = iter->node.get<Subroutine*>()->type;
      if(subrType->ownerStruct)
      {
        return subrType->ownerStruct;
      }
      else
      {
        //in a static subroutine, which is always a static context
        break;
      }
    }
  }
  return nullptr;
}

StructType* Scope::getMemberContext()
{
  //walk up scope tree, looking for a Struct scope before
  //reaching a static subroutine or global scope
  for(Scope* iter = this; iter; iter = iter->parent)
  {
    if(iter->node.is<Struct*>())
    {
      return iter->node.get<Struct*>();
    }
    else if(!iter->node.is<Module*>())
    {
      break;
    }
  }
  return nullptr;
}

Scope* Scope::getFunctionContext()
{
  for(Scope* iter = this; iter; iter = iter->parent)
  {
    if(iter->node.is<Subroutine*>())
    {
      auto subr = iter->node.get<Subroutine*>();
      if(subr->type->pure)
      {
        if(subr->type->ownerStruct)
        {
          return subr->type->ownerStruct->scope;
        }
        else
        {
          return subr->scope;
        }
      }
    }
  }
  return nullptr;
}

bool Scope::contains(Scope* other)
{
  for(Scope* iter = other; iter; iter = iter->parent)
  {
    if(iter == this)
      return true;
  }
  return false;
}

Module(string n, Scope* s)
{
  name = n;
  scope = new Scope(s, this);
}

