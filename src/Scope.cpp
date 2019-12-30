#include "Scope.hpp"
#include "TypeSystem.hpp"
#include "Variable.hpp"
#include "Subroutine.hpp"
#include "SourceFile.hpp"

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
Name::Name(SimpleType* t, Scope* s)
  : kind(SIMPLE_TYPE), name(t->name), scope(s)
{
  item = t;
}
Name::Name(AliasType* a, Scope* s)
  : kind(TYPEDEF), name(a->name), scope(s)
{
  item = a;
}
Name::Name(SubroutineDecl* subr, Scope* s)
  : kind(SUBROUTINE), name(subr->name), scope(s)
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

/*********/
/* Scope */
/*********/

Scope::Scope(Scope* p, Module* m) : parent(p), node(m)
{
  if(p) p->children.push_back(this);
}
Scope::Scope(Scope* p, StructType* s) : parent(p), node(s)
{
  if(p) p->children.push_back(this);
}
Scope::Scope(Scope* p, Subroutine* s) : parent(p), node(s)
{
  if(p) p->children.push_back(this);
}
Scope::Scope(Scope* p, Block* b) : parent(p), node(b)
{
  if(p) p->children.push_back(this);
}
Scope::Scope(Scope* p, EnumType* e) : parent(p), node(e)
{
  if(p) p->children.push_back(this);
}

void Scope::addName(const Name& n)
{
  //Check for name conflicts:
  //  No name can be redefined in the same scope,
  //  but names can shadow if they aren't in a block/subr.
  Name prev = lookup(n.name, false);
  if(prev.item)
  {
    errMsgLoc(n.item, "declaration " << n.name << " conflicts with other declaration at " << prev.item->printLocation());
  }
  if(node.is<Block*>() || node.is<Subroutine*>())
  {
    //Subr-local names can't shadow anything
    prev = findName(n.name);
    if(prev.item)
    {
      errMsgLoc(n.item, "local declaration " << n.name << " shadows a global declaration at " << prev.item->printLocation());
    }
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
IMPL_ADD_NAME(AliasType)
IMPL_ADD_NAME(SimpleType)
IMPL_ADD_NAME(EnumType)
IMPL_ADD_NAME(EnumConstant)
IMPL_ADD_NAME(SubroutineDecl)

void Scope::resolveAllUsings()
{
  for(auto ud : usingDecls)
    ud->resolve();
  for(auto child : children)
    child->resolveAllUsings();
}

void Scope::resolveAll()
{
  for(auto& name : names)
  {
    name.second.item->resolve();
  }
}

string Scope::getLocalName()
{
  if(node.is<Module*>())
    return node.get<Module*>()->name;
  if(node.is<StructType*>())
    return node.get<StructType*>()->name;
  if(node.is<Subroutine*>())
    return node.get<Subroutine*>()->decl->name;
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

Name Scope::lookup(const string& name, bool allowUsing)
{
  auto it = names.find(name);
  if(it != names.end())
    return it->second;
  else
  {
    //look in using decls
    if(allowUsing)
    {
      for(auto us : usingDecls)
      {
        Name n = us->lookup(name);
        if(n.item)
          return n;
      }
    }
  }
  //return "null" meaning not found
  return Name();
}

Name Scope::findName(Member* mem, bool allowUsing)
{
  //scope is the scope that actually contains name mem->tail
  Scope* scope = this;
  for(size_t i = 0; i < mem->names.size(); i++)
  {
    Name it = scope->lookup(mem->names[i], allowUsing);
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
        scope = ((Module*) it.item)->scope;
        continue;
      }
      else if(it.kind == Name::STRUCT)
      {
        scope = ((StructType*) it.item)->scope;
        continue;
      }
      else
      {
        //Have more names to look up, but 'it' does not correspond to a scope.
        //This is an error - the first name foudn shoudl 
        errMsgLoc(mem, "Name " << it.name <<
            " found but does not correspond to a scope, so "
            << mem->names[i + 1] << " cannot be a member of it");
      }
    }
    else
    {
      if(i == 0)
      {
        //First name not found - not an error
        scope = nullptr;
        break;
      }
      else
      {
        //Subsequent name not foudn - this is an error, since the
        //earlier names are the definitive matches
        errMsgLoc(mem, "Name " << mem->names[i] <<
            " was not declared");
      }
    }
  }
  //recursively try to search again in parent scope
  if(parent)
    return parent->findName(mem, allowUsing);
  //failure
  return Name();
}

Name Scope::findName(const string& name, bool allowUsing)
{
  Member m;
  m.names.push_back(name);
  return findName(&m, allowUsing);
}

StructType* Scope::getStructContext()
{
  //walk up scope tree, looking for a Struct scope before
  //reaching a static subroutine or global scope
  for(Scope* iter = this; iter; iter = iter->parent)
  {
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
        return nullptr;
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

bool Scope::isNestedModule()
{
  return node.is<Module*>() && (!parent || parent->isNestedModule());
}

/**********/
/* Module */
/**********/

Module::Module(string n, Scope* s)
{
  name = n;
  scope = new Scope(s, this);
}

void Module::resolveImpl()
{
  if(this == global)
  {
    scope->resolveAllUsings();
  }
  scope->resolveAll();
  resolved = true;
}

/***************/
/* UsingModule */
/***************/

UsingModule::UsingModule(Member* mname, Scope* s)
{
  moduleName = mname;
  scope = s;
}

void UsingModule::resolveImpl()
{
  //Find the module
  Name n = scope->findName(moduleName, false);
  if(!n.item)
  {
    errMsgLoc(this, *moduleName << " was not declared");
  }
  else if(n.kind != Name::MODULE)
  {
    errMsgLoc(this, *moduleName << " found at " << n.item->printLocation() << ", but is not a module");
  }
  module = (Module*) n.item;
  resolved = true;
}

Name UsingModule::lookup(const string& n)
{
  INTERNAL_ASSERT(resolved);
  return module->scope->lookup(n);
}

/*************/
/* UsingName */
/*************/

UsingName::UsingName(Member* n, Scope* s)
{
  fullName = n;
  scope = s;
}

void UsingName::resolveImpl()
{
  name = scope->findName(fullName, false);
  if(!name.item)
  {
    errMsgLoc(this, *fullName << " was not declared");
  }
  resolved = true;
}

Name UsingName::lookup(const string& n)
{
  if(name.name == n)
    return name;
  return Name();
}

