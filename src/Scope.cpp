#include "Scope.hpp"
#include "TypeSystem.hpp"
#include "Variable.hpp"
#include "Subroutine.hpp"

using namespace TypeSystem;

Scope* global;

bool Name::inScope(Scope* s)
{
  //see if scope is same as, or child of, s
  for(Scope* iter = scope; iter; iter = iter->parent)
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
Scope::Scope(Scope* p, StructType* s) : parent(p), node(s) {}
Scope::Scope(Scope* p, Subroutine* s) : parent(p), node(s) {}
Scope::Scope(Scope* p, Block* b) : parent(p), node(b) {}
Scope::Scope(Scope* p, EnumType* e) : parent(p), node(e) {}

void Scope::addName(Name n)
{
  Name prev = findName(n.name);
  if(prev.item)
  {
    errMsgLoc(n.item, "name " << n.name << " redefined (previous declaration at " << prev.item->printLocation());
  }
  names[n.name] = n;
}

#define IMPL_ADD_NAME(type) \
void Scope::addName(type* item) \
{ \
  addName(Name(item, this)); \
}

IMPL_ADD_NAME(Variable)
IMPL_ADD_NAME(Module)
IMPL_ADD_NAME(StructType)
IMPL_ADD_NAME(Subroutine)
IMPL_ADD_NAME(ExternalSubroutine)
IMPL_ADD_NAME(AliasType)
IMPL_ADD_NAME(EnumType)
IMPL_ADD_NAME(EnumConstant)

Name::Name(Module* m, Scope* parent)
  : kind(MODULE), name(m->name), scope(parent)
{
  item = m;
}
Name::Name(StructType* st, Scope* s)
  : kind(STRUCT), name(st->name), scope(s)
{
  item = st;
}
Name::Name(EnumType* e, Scope* s)
  : kind(ENUM), name(e->name), scope(s)
{
  item = e;
}
Name::Name(AliasType* a, Scope* s)
  : kind(TYPEDEF), name(a->name), scope(s)
{
  item = a;
}
Name::Name(Subroutine* subr, Scope* s)
  : kind(SUBROUTINE), name(subr->name), scope(s)
{
  item = subr;
}
Name::Name(ExternalSubroutine* subr, Scope* s)
  : kind(EXTERN_SUBR), name(subr->name), scope(s)
{
  item = subr;
}
Name::Name(Variable* var, Scope* s)
  : kind(VARIABLE), name(var->name), scope(s)
{
  item = var;
}
Name::Name(EnumConstant* ec, Scope* s)
  : kind(ENUM_CONSTANT), name(ec->name), scope(s)
{
  item = ec;
}

string Scope::getLocalName()
{
  if(node.is<Module*>())
    return node.get<Module*>()->name;
  if(node.is<StructType*>())
    return node.get<StructType*>()->name;
  if(node.is<Subroutine*>())
    return node.get<Subroutine*>()->name;
  if(node.is<Block*>())
  {
    Oss oss;
    //just show the raw Block pointer, not useful
    //but this shouldn't be shown to user anyway
    oss << "<Block " << node.get<Block*>() << '>';
    return oss.str();
  }
  if(node.is<EnumType*>())
    return node.get<EnumType*>()->name;
  INTERNAL_ERROR;
  return "";
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

Name Scope::findName(Member* mem)
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
        scope = ((StructType*) it.item)->scope;
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
  Member m;
  m.names.push_back(name);
  return findName(&m);
}

StructType* Scope::getStructContext()
{
  //walk up scope tree, looking for a Struct scope before
  //reaching a static subroutine or global scope
  for(Scope* iter = this; iter; iter = iter->parent)
  {
    if(iter->node.is<StructType*>())
    {
      return iter->node.get<StructType*>();
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
    if(iter->node.is<StructType*>())
    {
      return iter->node.get<StructType*>();
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

Module::Module(string n, Scope* s)
{
  name = n;
  scope = new Scope(s, this);
}

