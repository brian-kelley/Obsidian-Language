#include "TypeSystem.hpp"
//Include Expression here because it includes TypeSystem.hpp
#include "Expression.hpp"

using namespace Parser;

/***********************/
/* Type and subclasses */
/***********************/

struct ModuleScope;

extern ModuleScope* global;
TType* TType::inst;

namespace TypeSystem
{

vector<Type*> primitives;
map<string, Type*> primNames;
vector<TupleType*> tuples;
vector<ArrayType*> arrays;
vector<CallableType*> callables;

DeferredTypeLookup* typeLookup;
DeferredTraitLookup* traitLookup;

Type::Type(Scope* enclosingScope) : enclosing(enclosingScope) {}

bool Type::canConvert(Expression* other)
{
  //Basic behavior here: if other has a known type, check if that can convert
  if(other->type)
    return canConvert(other->type);
  return false;
}

Type* Type::getArrayType(int dims)
{
  //lazily check & create array type
  if((int) dimTypes.size() < dims)
  {
    //create + add
    for(int i = dimTypes.size() + 1; i <= dims; i++)
    {
      dimTypes.push_back(new ArrayType(this, i));
    }
  }
  return dimTypes[dims - 1];
}

void createBuiltinTypes()
{
  using Parser::TypeNT;
  //primitives has same size as the enum Parser::TypeNT::Prim
  primitives.resize(13);
  primitives[TypeNT::BOOL] = new BoolType;
  primitives[TypeNT::CHAR] = new CharType;
  primitives[TypeNT::BYTE] = new IntegerType("byte", 1, true);
  primitives[TypeNT::UBYTE] = new IntegerType("ubyte", 1, false);
  primitives[TypeNT::SHORT] = new IntegerType("short", 2, true);
  primitives[TypeNT::USHORT] = new IntegerType("ushort", 2, false);
  primitives[TypeNT::INT] = new IntegerType("int", 4, true);
  primitives[TypeNT::UINT] = new IntegerType("uint", 4, false);
  primitives[TypeNT::LONG] = new IntegerType("long", 8, true);
  primitives[TypeNT::ULONG] = new IntegerType("ulong", 8, false);
  primitives[TypeNT::FLOAT] = new FloatType("float", 4);
  primitives[TypeNT::DOUBLE] = new FloatType("double", 8);
  primitives[TypeNT::VOID] = new VoidType;
  primNames["bool"] = primitives[TypeNT::BOOL];
  primNames["char"] = primitives[TypeNT::CHAR];
  primNames["byte"] = primitives[TypeNT::BYTE];
  primNames["ubyte"] = primitives[TypeNT::UBYTE];
  primNames["short"] = primitives[TypeNT::SHORT];
  primNames["ushort"] = primitives[TypeNT::USHORT];
  primNames["int"] = primitives[TypeNT::INT];
  primNames["uint"] = primitives[TypeNT::UINT];
  primNames["long"] = primitives[TypeNT::LONG];
  primNames["ulong"] = primitives[TypeNT::ULONG];
  primNames["float"] = primitives[TypeNT::FLOAT];
  primNames["double"] = primitives[TypeNT::DOUBLE];
  primNames["void"] = primitives[TypeNT::VOID];
  //string is a builtin alias for char[] (not a primitive)
  global->types.push_back(new AliasType(
        "string", primitives[TypeNT::CHAR]->getArrayType(1), global));
  TType::inst = new TType;
  global->types.push_back(new AliasType("i8", primitives[TypeNT::BYTE], global));
  global->types.push_back(new AliasType("u8", primitives[TypeNT::UBYTE], global));
  global->types.push_back(new AliasType("i16", primitives[TypeNT::SHORT], global));
  global->types.push_back(new AliasType("u16", primitives[TypeNT::USHORT], global));
  global->types.push_back(new AliasType("i32", primitives[TypeNT::INT], global));
  global->types.push_back(new AliasType("u32", primitives[TypeNT::UINT], global));
  global->types.push_back(new AliasType("i64", primitives[TypeNT::LONG], global));
  global->types.push_back(new AliasType("u64", primitives[TypeNT::ULONG], global));
  global->types.push_back(new AliasType("f32", primitives[TypeNT::FLOAT], global));
  global->types.push_back(new AliasType("f64", primitives[TypeNT::DOUBLE], global));
}

string typeErrorMessage(TypeLookup& lookup)
{
  return "";
}

Type* lookupType(Parser::TypeNT* type, Scope* scope)
{
  //handle array immediately - just make an array and then handle the singular element type
  if(type->arrayDims)
  {
    size_t dims = type->arrayDims;
    type->arrayDims = 0;
    //now look up the type for the element type
    Type* elemType = lookupType(type, scope);
    //restore original type to preserve AST
    type->arrayDims = dims;
    if(!elemType)
    {
      //elem lookup type failed, so wait to get the array type
      return NULL;
    }
    else
    {
      return elemType->getArrayType(dims);
    }
  }
  else if(type->t.is<TypeNT::Prim>())
  {
    return primitives[(int) type->t.get<TypeNT::Prim>()];
  }
  else if(type->t.is<Member*>())
  {
    auto mem = type->t.get<Member*>();
    auto typeSearch = scope->findSub(mem->scopes);
    for(auto s : typeSearch)
    {
      for(auto t : s->types)
      {
        if(t->getName() == mem->ident)
        {
          if(AliasType* at = dynamic_cast<AliasType*>(t))
          {
            return at->actual;
          }
          return t;
        }
      }
    }
    return nullptr;
  }
  else if(type->t.is<TupleTypeNT*>())
  {
    //get a list of member types
    vector<Type*> members;
    for(auto mem : type->t.get<TupleTypeNT*>()->members)
    {
      members.push_back(lookupType(mem, scope));
      if(members.back() == nullptr)
      {
        return nullptr;
      }
    }
    for(auto tt : tuples)
    {
      if(tt->matchesTypes(members))
      {
        return tt;
      }
    }
    TupleType* newTuple = new TupleType(members);
    tuples.push_back(newTuple);
    return newTuple;
  }
  else if(type->t.is<SubroutineTypeNT*>())
  {
    SubroutineTypeNT* subr = type->t.get<SubroutineTypeNT*>();
  }
  else if(type->t.is<TraitType*>())
  {
    //look up the traits, then sort the pointers
    //find existing bounded type that matches, or create new one
    //if lookup of any trait fails, must return NULL for now
    //Note: Trait types 
    vector<Trait*> typeTraits;
    auto tt = type->t.get<TraitType*>();
    for(auto trait : tt->traits)
    {
      //using deferred trait lookup here
      Trait* 
    }
  }
  return nullptr;
}

Type* lookupTypeDeferred(TypeLookup& args)
{
  return lookupType(args.type, args.scope);
}

Trait* lookupTypeDeferred(TypeLookup& args)
{
  return lookupTrait(args.type, args.scope);
}

Type* getIntegerType(int bytes, bool isSigned)
{
  using Parser::TypeNT;
  switch(bytes)
  {
    case 1:
      if(isSigned)  return primitives[TypeNT::BYTE];
      else          return primitives[TypeNT::UBYTE];
    case 2:
      if(isSigned)  return primitives[TypeNT::SHORT];
      else          return primitives[TypeNT::USHORT];
    case 4:
      if(isSigned)  return primitives[TypeNT::INT];
      else          return primitives[TypeNT::UINT];
    case 8:
      if(isSigned)  return primitives[TypeNT::LONG];
      else          return primitives[TypeNT::ULONG];
    default: INTERNAL_ERROR;
  }
  return NULL;
}

/****************/
/* Bounded Type */
/****************/

BoundedType::BoundedType(Parser::TraitType* tt, Scope* s) : Type(NULL)
{
  traits.resize(tt->traits.size());
  for(size_t i = 0; i < tt->traits.size(); i++)
  {
    traits[i] = getTrait(tt->traits[i], s, &traits[i], false);
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
    if(callable.is<FuncDecl*>())
      numFuncs++;
    else
      numProcs++;
  }
  funcs.resize(numFuncs);
  procs.resize(numProcs);
  int funcIndex = 0;
  int procIndex = 0;
  //now, look up
  for(auto& callable : td->members)
  {
    if(callable.is<FuncDecl*>())
    {
      auto fdecl = callable.get<FuncDecl*>();
      funcs[funcIndex].type = getFuncType(&fdecl->type, s, (Type**) &funcs[funcIndex].type, false);
      funcs[funcIndex].name = fdecl->name;
      funcIndex++;
    }
    else
    {
      auto pdecl = callable.get<ProcDecl*>();
      procs[procIndex].type = getProcType(&pdecl->type, s, (Type**) &procs[procIndex].type, false);
      procs[procIndex].name = pdecl->name;
      procIndex++;
    }
  }
}

/***************/
/* Struct Type */
/***************/

StructType::StructType(Parser::StructDecl* sd, Scope* enclosingScope, StructScope* sscope) : Type(enclosingScope)
{
  this->name = sd->name;
  //can't actually handle any members yet - need to visit this struct decl as a scope first
  //but, this happens later
  decl = sd;
  //must assume there are unresolved members
  this->structScope = sscope;
  //Need to size members immediately (so the vector is never reallocated again)
  //Count the struct members which are VarDecls
  size_t numMemberVars = 0;
  for(auto& it : sd->members)
  {
    if(it->sd->decl.is<VarDecl*>())
      numMemberVars++;
  }
  members.resize(numMemberVars);
  memberNames.resize(numMemberVars);
  size_t membersAdded = 0;
  for(auto& it : sd->members)
  {
    if(it->sd->decl.is<VarDecl*>())
    {
      VarDecl* data = it->sd->decl.get<VarDecl*>();
      //lookup struct member types inside the struct's scope
      TypeLookup lookupArgs(data->type, structScope);
      typeLookup->lookup(lookupArgs, members[membersAdded]);
      memberNames[membersAdded] = data->name;
      membersAdded++;
    }
  }
  //Load traits
  traits.resize(sd->traits.size());
  for(size_t i = 0; i < sd->traits.size(); i++)
  {
    traits[i] = getTrait(sd->traits[i], enclosingScope, &traits[i], false);
  }
}

//direct conversion requires other to be the same type
bool StructType::canConvert(Type* other)
{
  return other == this;
}

bool StructType::canConvert(Expression* other)
{
  if(other->type == this)
    return true;
  else if(other->type != nullptr)
    return false;
  //if compound literal or tuple literal, check if those match members
  CompoundLiteral* cl = dynamic_cast<CompoundLiteral*>(other);
  if(cl)
  {
    if(cl->members.size() != members.size())
    {
      return false;
    }
    bool canConvert = true;
    for(size_t i = 0; i < members.size(); i++)
    {
      if(!(members[i]->canConvert(cl->members[i])))
      {
        canConvert = false;
        break;
      }
    }
    return canConvert;
  }
  return false;
}

bool StructType::implementsAllTraits()
{
  //go through each trait, and make sure there is an exactly matching
  //subroutine (names, 
}

/**************/
/* Union Type */
/**************/

UnionType::UnionType(Parser::UnionDecl* ud, Scope* enclosingScope) : Type(enclosingScope)
{
  decl = ud;
  name = ud->name;
  options.resize(ud->types.size());
  for(size_t i = 0; i < ud->types.size(); i++)
  {
    TypeLookup lookupArgs(ud->types[i], enclosingScope);
    typeLookup->lookup(lookupArgs, options[i]);
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

ArrayType::ArrayType(Type* elemType, int ndims) : Type(NULL)
{
  assert(elemType);
  this->dims = ndims;
  this->elem = elemType;
  if(ndims == 1)
  {
    this->subtype = elemType;
  }
  else
  {
    this->subtype = this->elem->dimTypes[ndims - 2];
  }
  //If an ArrayType is being constructed, it should always be the next dimension for elemType
  assert((int) elemType->dimTypes.size() == ndims - 1);
}

bool ArrayType::canConvert(Type* other)
{
  if(other->isArray())
  {
    ArrayType* at = (ArrayType*) other;
    //unlike C, allow implicit conversion of elements
    return dims == at->dims && elem->canConvert(at->elem);
  }
  else if(other->isTuple())
  {
    //Tuples can also be implicitly converted to arrays, as long as each member can be converted
    auto tt = dynamic_cast<TupleType*>(other);
    for(auto m : tt->members)
    {
      if(!(elem->canConvert(m)))
      {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool ArrayType::canConvert(Expression* other)
{
  if(other->type && canConvert(other->type))
  {
    return true;
  }
  CompoundLiteral* cl = dynamic_cast<CompoundLiteral*>(other);
  if(cl)
  {
    for(auto m : cl->members)
    {
      if(!(elem->canConvert(m)))
      {
        return false;
      }
    }
    return true;
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

TupleType::TupleType(vector<Type*> mems) : Type(NULL)
{
  this->members = mems;
}

bool TupleType::canConvert(Type* other)
{
  //true if other is identical or if this is a singleton and other can be converted to this's only member 
  return (this == other) || (members.size() == 1 && members[0]->canConvert(other));
}

bool TupleType::canConvert(Expression* other)
{
  if(other->type && canConvert(other->type))
    return true;
  CompoundLiteral* cl = dynamic_cast<CompoundLiteral*>(other);
  if(cl)
  {
    if(cl->members.size() != members.size())
    {
      return false;
    }
    for(size_t i = 0; i < members.size(); i++)
    {
      if(!(members[i]->canConvert(cl->members[i])))
      {
        return false;
      }
    }
    return true;
  }
  return false;
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

AliasType::AliasType(Typedef* td, Scope* scope) : Type(scope)
{
  name = td->ident;
  decl = td;
  TypeLookup args = TypeLookup(td->type, scope);
  typeLookup->lookup(args, actual);
}

AliasType::AliasType(string alias, Type* underlying, Scope* scope) : Type(scope)
{
  name = alias;
  actual = underlying;
  decl = NULL;
}

bool AliasType::canConvert(Type* other)
{
  return actual->canConvert(other);
}

bool AliasType::canConvert(Expression* other)
{
  return actual->canConvert(other);
}

/*************/
/* Enum Type */
/*************/

EnumType::EnumType(Parser::Enum* e, Scope* current) : Type(current)
{
  name = e->name;
  set<int64_t> usedVals;
  vector<int64_t> vals(e->items.size(), 0);
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
        ERR_MSG(errMsg);
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
  if(vals.size() == 0)
  {
    bytes = 1;
  }
  else
  {
    //get the min and max values
    int64_t minVal = vals[0];
    int64_t maxVal = vals[0];
    for(size_t i = 1; i < vals.size(); i++)
    {
      if(vals[i] < minVal)
        minVal = vals[i];
      if(vals[i] > maxVal)
        maxVal = vals[i];
    }
    int64_t absMax = std::max(-minVal, maxVal);
    if(absMax <= 0xFF)
      bytes = 1;
    else if(absMax <= 0xFFFF)
    {
      bytes = 2;
    }
    else if(absMax <= 0xFFFFFFFF)
    {
      bytes = 4;
    }
    else
    {
      bytes = 8;
    }
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

bool EnumType::isPrimitive()
{
  return true;
}

/****************/
/* Integer Type */
/****************/

IntegerType::IntegerType(string typeName, int sz, bool sign) : Type(global)
{
  this->name = typeName;
  this->size = sz;
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

bool IntegerType::isPrimitive()
{
  return true;
}

/**************/
/* Float Type */
/**************/

FloatType::FloatType(string typeName, int sz) : Type(global)
{
  this->name = typeName;
  this->size = sz;
}

bool FloatType::canConvert(Type* other)
{
  return other->isNumber();
}

bool FloatType::isNumber()
{
  return true;
}

bool FloatType::isPrimitive()
{
  return true;
}

/*************/
/* Char Type */
/*************/

bool CharType::canConvert(Type* other)
{
  return other->isInteger();
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

bool BoolType::isPrimitive()
{
  return true;
}

/*************/
/* Void Type */
/*************/

VoidType::VoidType() : Type(global) {}

bool VoidType::canConvert(Type* t)
{
  return t->isVoid();
}

bool VoidType::isVoid()
{
  return true;
}

bool VoidType::isPrimitive()
{
  return true;
}

/*****************/
/* Callable Type */
/*****************/

CallableType::CallableType(StructType* thisT, bool isPure, Type* retType, vector<Type*>& args) : Type(NULL)
{
  this->returnType = retType;
  this->pure = isPure;
  this->thisType = thisT;
  this->argTypes = args;
}

bool CallableType::matches(StructType* thisT, bool isPure, Type* returnT, vector<Type*>& args)
{
  if(pure != isPure)
    return false;
  //for callable, all types must match exactly (no implicit conversions)
  if(thisType != thisT || this->returnType != returnT)
    return false;
  if(argTypes.size() != args.size())
    return false;
  for(size_t i = 0; i < args.size(); i++)
  {
    if(argTypes[i] != args[i])
      return false;
  }
  return true;
}

CallableType* CallableType::lookup(StructType* thisT, bool isPure, Type* returnType, vector<Type*>& args)
{
  for(auto ct : callables)
  {
    if(ct->matches(thisT, isPure, returnT, args))
      return ct;
  }
  //need to create a new type
  CallableType* newType = new CallableType(thisT, isPure, returnType, args);
  callables.push_back(newType);
  return newType;
}

} //namespace TypeSystem

