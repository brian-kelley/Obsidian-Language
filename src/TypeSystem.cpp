#include "TypeSystem.hpp"

using namespace std;
using namespace Parser;

/***********************/
/* Type and subclasses */
/***********************/

struct ModuleScope;

extern ModuleScope* global;

namespace TypeSystem
{

vector<Type*> primitives;
vector<TupleType*> tuples;
vector<ArrayType*> arrays;
vector<UnresolvedType> unresolved;
vector<UnresolvedTrait> unresolvedTraits;

Type::Type(Scope* enclosingScope)
{
  //Syntactic types (arrays, tuples, functions, procedures) have no scope, so enclosingScope can be null
  if(enclosingScope)
  {
    enclosing = enclosingScope;
    enclosing->types.push_back(this);
  }
}

void createBuiltinTypes()
{
  primitives.push_back(new BoolType);
  primitives.emplace_back(new IntegerType("char", 1, true));
  new AliasType("i8", primitives.back(), global);
  primitives.emplace_back(new IntegerType("uchar", 1, false));
  new AliasType("u8", primitives.back(), global);
  primitives.emplace_back(new IntegerType("short", 2, true));
  new AliasType("i16", primitives.back(), global);
  primitives.emplace_back(new IntegerType("ushort", 2, false));
  new AliasType("u16", primitives.back(), global);
  primitives.emplace_back(new IntegerType("int", 4, true));
  new AliasType("i32", primitives.back(), global);
  primitives.emplace_back(new IntegerType("uint", 4, false));
  new AliasType("u32", primitives.back(), global);
  primitives.emplace_back(new IntegerType("long", 8, true));
  new AliasType("i64", primitives.back(), global);
  primitives.emplace_back(new IntegerType("ulong", 8, false));
  new AliasType("u64", primitives.back(), global);
  primitives.emplace_back(new FloatType("float", 4));
  new AliasType("f32", primitives.back(), global);
  primitives.emplace_back(new FloatType("double", 8));
  new AliasType("f64", primitives.back(), global);
  primitives.emplace_back(new StringType);
}

Type* getType(Parser::TypeNT* type, Scope* usedScope, Type** usage, bool failureIsError)
{
  //handle array immediately - just make an array and then handle the singular element type
  if(type->arrayDims)
  {
    size_t dims = type->arrayDims;
    type->arrayDims = 0;
    //now look up the type for the element type
    Type* elemType = getType(type, usedScope, nullptr, false);
    //restore original type to preserve AST
    type->arrayDims = dims;
    if(elemType)
    {
      //lazily check & create array type
      if(elemType->dimTypes.size() >= dims)
      {
        //already exists
        return elemType->dimTypes[dims - 1];
      }
      else
      {
        //create + add
        //size = 1 -> max dim = 1
        for(size_t i = elemType->dimTypes.size() + 1; i <= dims; i++)
        {
          new ArrayType(elemType, i);
        }
        //now return the needed type
        return elemType->dimTypes.back();
      }
    }
    else if(usage)
    {
      unresolved.emplace_back(type, usedScope, usage);
      return nullptr;
    }
    else if(failureIsError)
    {
      errAndQuit("Required ArrayType but its element type could not be resolved.");
    }
  }
  else if(type->t.is<TypeNT::Prim>())
  {
    return primitives[(int) type->t.get<TypeNT::Prim>()];
  }
  else if(type->t.is<AP(Member)>())
  {
    //search up scope tree for the member
    //need to search for EnumType, AliasType, StructType or UnionType
    for(Scope* iter = usedScope; iter; iter = iter->parent)
    {
      Scope* memScope = iter;
      //iter is the root of search (scan for child scopes, then the type)
      for(Member* search = type->t.get<AP(Member)>().get();
          search; search = search->mem.get())
      {
        bool foundNext = false;
        if(search->mem)
        {
          //find scope with name mem->owner
          for(auto& searchScope : memScope->children)
          {
            if(searchScope->getLocalName() == search->owner)
            {
              //found the next memScope for searching

              foundNext = true;
              memScope = searchScope;
              break;
            }
          }
          if(!foundNext)
          {
            //stop searching down this chain of scopes
            break;
          }
        }
        else
        {
          //find type with name mem->owner
          for(auto& searchType : iter->types)
          {
            StructType* st = dynamic_cast<StructType*>(searchType);
            if(st && st->name == search->owner)
            {
              return st;
            }
            UnionType* ut = dynamic_cast<UnionType*>(searchType);
            if(ut && ut->name == search->owner)
            {
              return ut;
            }
            EnumType* et = dynamic_cast<EnumType*>(searchType);
            if(et && et->name == search->owner)
            {
              return et;
            }
            AliasType* at = dynamic_cast<AliasType*>(searchType);
            if(at && at->name == search->owner)
            {
              return at;
            }
          }
        }
      }
    }
    //couldn't find type, mark as unresolved
    //If usage is null (meaning this is during the resolving pass), is fatal error
    if(failureIsError)
    {
      string msg = "Could not resolve type: \"";
      auto mem = type->t.get<AP(Member)>();
      //walk down the member chain (print owner, then '.', then remainder)
      Parser::Member* it = mem.get();
      for(; it->mem; it = it->mem.get())
      {
        msg += it->owner;
        msg += '.';
      }
      msg += it->owner;
      msg += "\" required from scope \"";
      msg += usedScope->getLocalName();
      msg += "\"";
      errAndQuit(msg);
    }
    if(usage)
      unresolved.emplace_back(type, usedScope, usage);
  }
  else if(type->t.is<AP(TupleTypeNT)>())
  {
    auto& tt = type->t.get<AP(TupleTypeNT)>();
    //search for each member individually
    bool resolved = true;
    //TupleTypes can have unresolved arrays/tuples as members, so is necessary to pass usage ptr for each member
    vector<Type*> types(tt->members.size(), nullptr);
    size_t i;
    for(i = 0; i < tt->members.size(); i++)
    {
      auto& mem = tt->members[i];
      types[i] = getType(mem.get(), usedScope, nullptr, false);
      if(!types[i])
        resolved = false;
    }
    if(!resolved)
    {
      if(failureIsError)
      {
        errAndQuit(string("Required TupleType but member ") + to_string(i) + " could not be resolved.");
      }
      else if(usage)
      {
        unresolved.emplace_back(type, usedScope, usage);
        return nullptr;
      }
    }
    for(auto& existing : tuples)
    {
      if(existing->matchesTypes(types))
      {
        return existing;
      }
    }
    //must create new type
    return new TupleType(types);
  }
  else if(type->t.is<AP(FuncTypeNT)>())
  {
    return getFuncType(type->t.get<AP(FuncTypeNT)>().get(), usedScope, usage, failureIsError);
  }
  else if(type->t.is<AP(ProcTypeNT)>())
  {
    return getProcType(type->t.get<AP(ProcTypeNT)>().get(), usedScope, usage, failureIsError);
  }
  else if(type->t.is<AP(TraitType)>())
  {
    return new BoundedType(type->t.get<AP(TraitType)>().get(), usedScope);
  }
  return nullptr;
}

FuncType* getFuncType(Parser::FuncTypeNT* type, Scope* usedScope, Type** usage, bool failureIsError)
{
  //TODO: cache these and check for existing type before creating new one
  return new FuncType(type, usedScope);
}

ProcType* getProcType(Parser::ProcTypeNT* type, Scope* usedScope, Type** usage, bool failureIsError)
{
  return new ProcType(type, usedScope);
}

Trait* getTrait(Parser::Member* name, Scope* usedScope, Trait** usage, bool failureIsError)
{
  //search up scope tree for the member
  //need to search for EnumType, AliasType, StructType or UnionType
  for(Scope* iter = usedScope; iter; iter = iter->parent)
  {
    Scope* memScope = iter;
    //iter is the root of search (scan for child scopes, then the type)
    for(Member* search = name;
        search; search = search->mem.get())
    {
      bool foundNext = false;
      if(search->mem)
      {
        //find scope with name mem->owner
        for(auto& searchScope : memScope->children)
        {
          if(searchScope->getLocalName() == search->owner)
          {
            //found the next memScope for searching
            foundNext = true;
            memScope = searchScope;
            break;
          }
        }
        if(!foundNext)
        {
          //stop searching down this chain of scopes
          break;
        }
      }
      else
      {
        for(auto searchTrait : iter->traits)
        {
          if(searchTrait->name == search->owner)
            return searchTrait;
        }
      }
    }
  }
  //couldn't find type, mark as unresolved
  //If usage is null (meaning this is during the resolving pass), is fatal error
  if(failureIsError)
  {
    string msg = "Could not resolve trait: \"";
    auto mem = name;
    //walk down the member chain (print owner, then '.', then remainder)
    Parser::Member* it = mem;
    for(; it->mem; it = it->mem.get())
    {
      msg += it->owner;
      msg += '.';
    }
    msg += it->owner;
    msg += "\" required from scope \"";
    msg += usedScope->getLocalName();
    msg += "\"";
    errAndQuit(msg);
  }
  if(usage)
    unresolvedTraits.emplace_back(name, usedScope, usage);
  return nullptr;
}

void resolveAllTypes()
{
  //is faster to resolve types in reverse order because long dependency chains can form
  for(int i = unresolved.size() - 1; i >= 0; i--)
  {
    auto& ut = unresolved[i];
    //note: failureIsError is true because all named types should be available now
    Type* t = nullptr;
    if(ut.parsedType)
      t = getType(ut.parsedType, ut.scope, nullptr, true);
    else if(ut.parsedFunc)
      t = getFuncType(ut.parsedFunc, ut.scope, nullptr, true);
    else
      t = getProcType(ut.parsedProc, ut.scope, nullptr, true);
    if(!t)
    {
      errAndQuit("Type could not be resolved.");
    }
    *(ut.usage) = t;
  }
}

void resolveAllTraits()
{
  for(auto& ut : unresolvedTraits)
  {
    //note: failureIsError is true because all named types should be available now
    Trait* t = getTrait(ut.parsed, ut.scope, nullptr, true);
Trait* getTrait(Parser::Member* name, Scope* usedScope, Trait** usage, bool failureIsError = true);
    if(!t)
    {
      errAndQuit("Trait could not be resolved.");
    }
    *(ut.usage) = t;
  }
}

/*****************/
/* Function Type */
/*****************/

FuncType::FuncType(Parser::FuncTypeNT* ft, Scope* scope) : Type(nullptr)
{
  retType = getType(ft->retType.get(), scope, &retType, false);
  for(size_t i = 0; i < ft->args.size(); i++)
  {
    argTypes.push_back(getType(ft->args[i]->type.get(), scope, &argTypes[i], false));
  }
}

bool FuncType::isCallable()
{
  return true;
}

bool FuncType::isFunc()
{
  return true;
}

bool FuncType::canConvert(Type* other)
{
  //True if other is also a function and has ret type and arg types that can be converted
  FuncType* fp = dynamic_cast<FuncType*>(other);
  if(fp == nullptr)
    return false;
  if(!retType->canConvert(fp->retType))
    return false;
  if(argTypes.size() != fp->argTypes.size())
    return false;
  for(size_t i = 0; i < argTypes.size(); i++)
  {
    if(!argTypes[i]->canConvert(fp->argTypes[i]))
      return false;
  }
  return true;
}

/******************/
/* Procedure Type */
/******************/

ProcType::ProcType(Parser::ProcTypeNT* pt, Scope* scope) : Type(nullptr)
{
  retType = getType(pt->retType.get(), scope, &retType, false);
  for(size_t i = 0; i < pt->args.size(); i++)
  {
    argTypes.push_back(getType(pt->args[i]->type.get(), scope, &argTypes[i], false));
  }
  nonterm = pt->nonterm;
}

bool ProcType::isCallable()
{
  return true;
}

bool ProcType::isProc()
{
  return true;
}

bool ProcType::canConvert(Type* other)
{
  //True if other is a callable and has ret type and arg types that can be converted
  FuncType* f = dynamic_cast<FuncType*>(other);
  ProcType* p = dynamic_cast<ProcType*>(other);
  if(f)
  {
    if(!retType->canConvert(f->retType))
      return false;
    if(argTypes.size() != f->argTypes.size())
      return false;
    for(size_t i = 0; i < argTypes.size(); i++)
    {
      if(!argTypes[i]->canConvert(f->argTypes[i]))
        return false;
    }
    return true;
  }
  else if(p)
  {
    //terminating procedures are a subset of the non-terminating procedures
    if(p->nonterm && !nonterm)
      return false;
    if(!retType->canConvert(p->retType))
      return false;
    if(argTypes.size() != p->argTypes.size())
      return false;
    for(size_t i = 0; i < argTypes.size(); i++)
    {
      if(!argTypes[i]->canConvert(p->argTypes[i]))
        return false;
    }
    return true;
  }
  return false;
}

/****************/
/* Bounded Type */
/****************/

BoundedType::BoundedType(Parser::TraitType* tt, Scope* s) : Type(nullptr)
{
  traits.resize(tt->traits.size());
  for(size_t i = 0; i < tt->traits.size(); i++)
  {
    traits[i] = getTrait(tt->traits[i].get(), s, &traits[i], false);
  }
}

/***********/
/*  Trait  */
/***********/

Trait::Trait(Parser::TraitDecl* td, Scope* s)
{
  //pre-allocate func and proc list (and their names)
  int numFuncs = 0;
  int numProcs = 0;
  for(auto& callable : td->members)
  {
    if(callable.is<AP(FuncDecl)>())
      numFuncs++;
    else
      numProcs++;
  }
  funcs.resize(numFuncs);
  funcNames.resize(numFuncs);
  procs.resize(numProcs);
  procNames.resize(numProcs);
  int funcIndex = 0;
  int procIndex = 0;
  //now, look up
  for(auto& callable : td->members)
  {
    if(callable.is<AP(FuncDecl)>())
    {
      auto fdecl = callable.get<AP(FuncDecl)>();
      funcs[funcIndex] = getFuncType(&fdecl->type, s, (Type**) &funcs[funcIndex], false);
      funcNames[funcIndex] = fdecl->name;
      funcIndex++;
    }
    else
    {
      auto pdecl = callable.get<AP(ProcDecl)>();
      procs[procIndex] = getProcType(&pdecl->type, s, (Type**) &procs[funcIndex], false);
      procNames[procIndex] = pdecl->name;
      procIndex++;
    }
  }
  s->traits.push_back(this);
}

/***************/
/* Struct Type */
/***************/

StructType::StructType(Parser::StructDecl* sd, Scope* enclosingScope, StructScope* structScope) : Type(enclosingScope)
{
  this->name = sd->name;
  //can't actually handle any members yet - need to visit this struct decl as a scope first
  //but, this happens later
  decl = sd;
  //must assume there are unresolved members
  this->structScope = structScope;
  //Need to size members immediately (so the vector is never reallocated again)
  //Count the struct members which are VarDecls
  size_t numMemberVars = 0;
  for(auto& it : sd->members)
  {
    if(it->sd->decl.is<AP(VarDecl)>())
      numMemberVars++;
  }
  members.resize(numMemberVars);
  memberNames.resize(numMemberVars);
  //TODO: actually handle the struct's traits here as well (also use 2nd pass for resolution)
  size_t membersAdded = 0;
  for(auto& it : sd->members)
  {
    if(it->sd->decl.is<AP(VarDecl)>())
    {
      VarDecl* data = it->sd->decl.get<AP(VarDecl)>().get();
      Type* dataType = getType(data->type.get(), structScope, &members[membersAdded], false);
      members[membersAdded] = dataType;
      memberNames[membersAdded] = data->name;
      membersAdded++;
    }
  }
}

bool StructType::hasFunc(FuncType* type)
{
  //TODO
  return false;
}

bool StructType::hasProc(ProcType* type)
{
  //TODO
  return false;
}

bool StructType::canConvert(Type* other)
{
  return other == this;
}

bool StructType::isStruct()
{
  return true;
}

/**************/
/* Union Type */
/**************/

UnionType::UnionType(Parser::UnionDecl* ud, Scope* enclosingScope) : Type(enclosingScope)
{
  decl = ud;
  name = ud->name;
  bool resolved = true;
  options.resize(ud->types.size());
  for(size_t i = 0; i < ud->types.size(); i++)
  {
    Type* option = getType(ud->types[i].get(), enclosingScope, &options[i], false);
    if(!option)
    {
      resolved = false;
    }
    options[i] = option;
  }
}

bool UnionType::canConvert(Type* other)
{
  return other == this;
}

bool UnionType::isUnion()
{
  return true;
}

/**************/
/* Array Type */
/**************/

ArrayType::ArrayType(Type* elemType, int dims) : Type(nullptr)
{
  assert(elemType);
  this->dims = dims;
  this->elem = elemType;
  //If an ArrayType is being constructed, it must be the next dimension for elemType
  assert(elemType->dimTypes.size() == dims - 1);
  elemType->dimTypes.push_back(this);
}

bool ArrayType::canConvert(Type* other)
{
  if(other->isArray())
  {
    ArrayType* at = (ArrayType*) other;
    //unlike C, allow implicit conversion of elements
    return dims == at->dims && elem->canConvert(at->elem);
  }
  return false;
}

bool ArrayType::isArray()
{
  return true;
}

/**************/
/* Tuple Type */
/**************/

TupleType::TupleType(vector<Type*> members) : Type(nullptr)
{
  this->members = members;
  tuples.push_back(this);
}

TupleType::TupleType(TupleTypeNT* tt, Scope* currentScope) : Type(nullptr)
{
  decl = tt;
  //Note: this constructor being called means that Type::getType
  //already successfully looked up all the members
  bool resolved = true;
  for(auto& it : tt->members)
  {
    TypeNT* typeNT = it.get();
    Type* type = getType(typeNT, currentScope, nullptr, false);
    if(!type)
    {
      resolved = false;
    }
    members.push_back(type);
  }
  tuples.push_back(this);
}

bool TupleType::canConvert(Type* other)
{
  return this == other;
}

bool TupleType::isTuple()
{
  return true;
}

bool TupleType::matchesTypes(vector<Type*>& types)
{
  return members.size() == types.size() && std::equal(types.begin(), types.end(), members.begin());
}

/**************/
/* Alias Type */
/**************/

AliasType::AliasType(Typedef* td, Scope* current) : Type(global)
{
  name = td->ident;
  decl = td;
  Type* t = getType(td->type.get(), current, &actual, false);
  actual = t;
}

AliasType::AliasType(string alias, Type* underlying, Scope* currentScope) : Type(currentScope)
{
  name = alias;
  actual = underlying;
  decl = nullptr;
}

bool AliasType::canConvert(Type* other)
{
  return actual->canConvert(other);
}

/*************/
/* Enum Type */
/*************/

EnumType::EnumType(Parser::Enum* e, Scope* current) : Type(current)
{
  name = e->name;
  set<int> usedVals;
  vector<int> vals(e->items.size(), 0);
  vector<bool> valsSet(e->items.size(), false);
  //first, process all specified values
  for(size_t i = 0; i < e->items.size(); i++)
  {
    auto& item = *e->items[i];
    if(item.value)
    {
      vals[i] = item.value->val;
      valsSet[i] = true;
      if(usedVals.find(vals[i]) == usedVals.end())
      {
        usedVals.insert(vals[i]);
      }
      else
      {
        string errMsg = "Enum \"";
        errMsg += e->name + "\" has a duplicate value " + to_string(vals[i]) + " with key \"" + item.name + "\"";
        errAndQuit(errMsg);
      }
    }
  }
  //now fill in remaining values automatically (start at 0)
  int autoVal = 0;
  for(size_t i = 0; i < e->items.size(); i++)
  {
    if(!valsSet[i])
    {
      //need a value for this key, pick one that hasn't been used already
      while(usedVals.find(autoVal) != usedVals.end())
      {
        autoVal++;
      }
      vals[i] = autoVal;
      usedVals.insert(autoVal);
    }
  }
  for(size_t i = 0; i < e->items.size(); i++)
  {
    values[e->items[i]->name] = vals[i];
  }
}

bool EnumType::canConvert(Type* other)
{
  return other->isInteger();
}

bool EnumType::isEnum()
{
  return true;
}

bool EnumType::isInteger()
{
  return true;
}

bool EnumType::isNumber()
{
  return true;
}

/****************/
/* Integer Type */
/****************/

IntegerType::IntegerType(string name, int size, bool sign) : Type(global)
{
  this->name = name;
  this->size = size;
  this->isSigned = sign;
}

bool IntegerType::canConvert(Type* other)
{
  return other->isEnum() || other->isInteger();
}

bool IntegerType::isInteger()
{
  return true;
}

bool IntegerType::isNumber()
{
  return true;
}

/**************/
/* Float Type */
/**************/

FloatType::FloatType(string name, int size) : Type(global)
{
  this->name = name;
  this->size = size;
}

bool FloatType::canConvert(Type* other)
{
  return other->isNumber();
}

bool FloatType::isNumber()
{
  return true;
}

/***************/
/* String Type */
/***************/

StringType::StringType() : Type(global) {}

bool StringType::canConvert(Type* other)
{
  return other->isString();
}

bool StringType::isString()
{
  return true;
}

/*************/
/* Bool Type */
/*************/

BoolType::BoolType() : Type(global) {}

bool BoolType::canConvert(Type* other)
{
  return other->isBool();
}

bool BoolType::isBool()
{
  return true;
}

} //namespace TypeSystem

