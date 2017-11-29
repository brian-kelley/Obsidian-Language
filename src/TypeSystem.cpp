#include "TypeSystem.hpp"
//Include Expression here because it includes TypeSystem.hpp
#include "Expression.hpp"

using namespace Parser;

/***********************/
/* Type and subclasses */
/***********************/

struct ModuleScope;

extern ModuleScope* global;

namespace TypeSystem
{
TType* TType::inst;

vector<Type*> primitives;
map<string, Type*> primNames;
set<ArrayType*, ArrayCompare< arrays;
set<TupleType*, TupleCompare> tuples;
set<UnionType*, UnionCompare> unions;
set<MapType*, MapCompare> maps;
set<CallableType*, CallableCompare> callables;

//these are created in MiddleEnd
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
  Oss oss;
  oss << "unknown type";
  return oss.str();
}

string traitErrorMessage(TraitLookup& lookup)
{
  Oss oss;
  oss << "unknown trait: " << *(lookup.name);
  return oss.str();
}

Type* lookupType(Parser::TypeNT* type, Scope* scope)
{
  //handle array immediately - just make an array and then handle the singular element type
  if(type->arrayDims)
  {
    int dims = type->arrayDims;
    //now look up the type for the element type
    type->arrayDims = 0;
    Type* elemType = lookupType(type, scope);
    //restore original type to preserve AST
    type->arrayDims = dims;
    if(!elemType)
    {
      //elem lookup type failed, so wait to get the array type
      return nullptr;
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
    //intercept special case: "T" inside a trait decl
    auto mem = type->t.get<Member*>();
    if(mem->head.size() == 0 && mem->tail.name == "T")
    {
      for(Scope* iter = scope; iter; iter = iter->parent)
      {
        if(auto ts = dynamic_cast<TraitScope*>(iter))
        {
          return TType::inst;
        }
      }
    }
    return scope->findType(mem);
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
  else if(type->t.is<UnionTypeNT*>())
  {
    auto utNT = type->t.get<UnionTypeNT*>();
    vector<TypeNT*> options;
    for(auto t : utNT->types)
    {
      TypeNT* next = lookupType(t, scope);
      if(!next)
      {
        //have to come back later when all option types are defined
        return nullptr;
      }
      options.push_back(next);
    }
    //special case: only one option (just return it)
    if(options.size() == 1)
      return options.front();
    UnionType* ut = new UnionType(options);
    //check if ut is already in the set of all union types
    auto it = unions.find(ut);
    if(it == unions.end())
    {
      //new union type, so add it to set
      unions.insert(ut);
      return ut;
    }
    else
    {
      //use type (which is already in the set)
      delete ut;
      return *it;
    }
  }
  else if(type->t.is<MapTypeNT*>())
  {
    auto mtNT = type->t.get<MapTypeNT*>();
    TypeNT* key = lookupType(mtNT->keyType, scope);
    TypeNT* value = lookupType(mtNT->valueType, scope);
    if(!key || !value)
      return nullptr;
    MapType* mt = new MapType(key, value);
    auto it = maps.find(mt);
    if(it == maps.end())
    {
      maps.insert(mt);
      return mt;
    }
    else
    {
      delete mt;
      return *it;
    }
  }
  else if(type->t.is<SubroutineTypeNT*>())
  {
    auto stNT = type->t.get<SubroutineTypeNT*>();
    //find the owner struct (from scope), if it exists
    StructScope* owner = nullptr;
    bool pure = dynamic_cast<FuncTypeNT*>(stNT);
    bool nonterm = true;
    if(auto pt = dynamic_cast<ProcTypeNT*>(stNT))
      nonterm = pt->nonterm;
    if(!stNT->isStatic)
    {
      //if scope is inside a StructScope, owner = corresponding struct type
      for(Scope* iter = scope; iter; iter = iter->parent)
      {
        if(auto ss = dynamic_cast<StructScope*>(iter))
        {
          owner = ss->type;
          break;
        }
      }
    }
    //get return type and argument types (return nullptr if those aren't available)
    Type* retType = lookupType(stNT->retType, scope);
    if(!retType)
      return nullptr;
    vector<Type*> argTypes;
    for(auto param : stNT->params)
    {
      if(param->type.is<TypeNT*>())
      {
        argTypes.push_back(lookupType(param->type.get<TypeNT*>(), scope));
        if(argTypes.back() == nullptr)
          return nullptr;
      }
      else
      {
        auto bt = param->type.get<BoundedTypeNT*>();
        //attempt to look up the bounded type by name
        Member m;
        m.ident = new Ident(bt->localName);
        TypeNT wrapper;
        wrapper.t = &m;
        TypeNT* namedBT = lookupType(&wrapper, scope);
        if(!namedBT)
          return nullptr;
        argTypes.push_back(namedBT);
      }
    }
    auto ct = new CallableType(pure, owner, retType, argTypes, nonterm);
    auto it = callables.find(ct);
    if(it == callables.end())
    {
      callables.insert(ct);
      return ct;
    }
    else
    {
      return *it;
    }
  }
  return nullptr;
}

CallableType* lookupSubroutineType(Parser::SubroutineTypeNT* subr, Scope* scope)
{
  //construct a TypeNT and then call lookupType
  Parser::TypeNT* wrapper = new Parser::TypeNT;
  wrapper->t = subr;
  CallableType* result = dynamic_cast<CallableType*>(lookupType(wrapper, scope));
  delete wrapper;
  return result;
}

Type* lookupTypeDeferred(TypeLookup& args)
{
  return lookupType(args.type, args.scope);
}

Trait* lookupTrait(Parser::Member* name, Scope* scope)
{
  return scope->findTrait(name);
}

Trait* lookupTraitDeferred(TraitLookup& args)
{
  return lookupTrait(args.name, args.scope);
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

bool BoundedType::canConvert(Expression* other)
{
  return other->type == this;
}

/***********/
/*  Trait  */
/***********/

Trait::Trait(Parser::TraitDecl* td, Scope* s)
{
  //pre-allocate subr and names vectors
  subrNames.resize(td->members.size());
  callables.resize(td->members.size());
  //use deferred type lookup for callables
  for(size_t i = 0; i < td->members.size(); i++)
  {
    auto mem = td->members[i];
    Parser::SubroutineTypeNT* subrType = nullptr;
    if(member.is<Parser::FuncDecl*>())
    {
      subrType = member.get<Parser::FuncDecl*>();
    }
    else
    {
      subrType = member.get<Parser::ProcDecl*>();
    }
    subrNames[i] = fd->name;
    TypeLookup lookupArgs(subrType, s);
    typeLookup->lookup(lookupArgs, callables[i]);
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
  //have struct scope point back to this
  sscope->type = this;
  //Need to size members immediately
  //(so the vector is never reallocated again, that would break deferred lookup)
  //Count the VarDecl members
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
  //Load traits using deferred trait lookup
  traits.resize(sd->traits.size());
  for(size_t i = 0; i < sd->traits.size(); i++)
  {
    TraitLookup lookupArgs(sd->traits[i], enclosingScope);
    traitLookup->lookup(lookupArgs, traits[i]);
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

bool StructType::implementsTrait(Trait* t)
{
  //note: requires that checking has already been done
  return find(traits.begin(), traits.end(), t) != traits.end();
}

/**************/
/* Union Type */
/**************/

UnionTypes::UnionType(vector<Type*> types) : Type(NULL)
{
  options = types;
  sort(options.begin(), options.end());
}

bool UnionType::canConvert(Type* other)
{
  return other == this;
}

string UnionType::getName()
{
  string name = "(";
  name += options[0]->getName();
  for(int i = 1; i < options.size(); i++)
  {
    name += " | ";
    name += options[i]->getName();
  }
  name += ")";
  return name;
}

bool UnionCompare::operator()(const UnionType* lhs, const UnionType* rhs)
{
  return lexicographical_compare(lhs->types.begin(), lhs->types.end(),
      rhs->types.begin(), rhs->types.end());
}

/**************/
/* Array Type */
/**************/

ArrayType::ArrayType(Type* elemType, int ndims) : Type(NULL)
{
  assert(elemType);
  assert(ndims > 0);
  this->dims = ndims;
  this->elem = elemType;
  subtype = ndims == 1 ? elem : elem->getArrayTypes(dims - 1);
}

ArrayType* ArrayType::getArrayType(int extradims)
{
  return elem->getArrayType(dims - 1);
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

bool ArrayType::operator()(const ArrayType* lhs, const ArrayType* rhs)
{
  if(lhs->elem < rhs->elem)
    return true;
  else if(lhs->elem == rhs->elem && lhs->dims < rhs->dims)
    return true;
  return false;
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

bool TupleCompare::operator()(const TupleType* lhs, const TupleType* rhs)
{
  return lexicographical_compare(lhs->members.begin(), lhs->members.end(),
      rhs->members.begin(), rhs->members.end());
}

/************/
/* Map Type */
/************/

bool MapType::operator()(const MapType* lhs, const MapType* rhs)
{
  return lhs->key < rhs->key || lhs->key == rhs->key && lhs->value < rhs->value;
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

/*************/
/* Void Type */
/*************/

VoidType::VoidType() : Type(global) {}

bool VoidType::canConvert(Type* t)
{
  return t->isVoid();
}

/*****************/
/* Callable Type */
/*****************/

CallableType::CallableType(bool isPure, Type* retType, vector<Type*>& args, bool term = true)
{
  pure = isPure;
  returnType = retType;
  argTypes = args;
  terminating = term;
  ownerStruct = NULL;
}

CallableType::CallableType(bool isPure, StructType* owner, Type* returnType, vector<Type*>& args, bool term = true)
{
  pure = isPure;
  returnType = retType;
  argTypes = args;
  terminating = term;
  ownerStruct = owner;
}

string CallableType::getName()
{
  Oss oss;
  if(pure)
    oss << "func ";
  else
    oss << "proc ";
  oss << returnType->getName();
  oss << "(";
  for(size_t i = 0; i < argTypes.size(); i++)
  {
    oss << argTypes[i]->getName();
    if(i < argTypes.size() - 1)
    {
      oss << ", ";
    }
  }
  oss << ")";
  return oss.str();
}

//all funcs can be procs
//all nonmember/static functions can
//  be member functions (by ignoring the this argument)
//member functions are only equivalent if they belong to same struct
//all terminating procedures can be used in place of nonterminating ones
bool CallableType::canConvert(Type* other)
{
  //Only CallableTypes are convertible to other CallableTypes
  auto ct = dynamic_cast<CallableType*>(other);
  if(!ct)
    return false;
  if(!ownerStruct && other->ownerStruct ||
      ownerStruct other->ownerStruct && ownerStruct != other->ownerStruct)
  {
    //this is static, but other is not
    //OR
    //both members but different owning structs
    return false;
  }
  if(!nonterminating && other->nonterminating)
    return false;
  if(isFunc() && ct->isProc())
    return false;
  //check that arguments are exactly the same
  //doing at end because more expensive test
  if(argTypes = other->argTypes)
    return false;
  return true;
}

bool CallableType::canConvert(Expression* other);
{
  return other->type && canConvert(other->type);
}

bool CallableCompare::operator()(const CallableType* lhs, const CallableType* rhs)
{
  //an arbitrary way to order all possible callables (is lhs < rhs?)
  if(!lhs->pure && rhs->pure)
    return true;
  if(!lhs->terminating && rhs->terminating)
    return true;
  if(lhs->returnType < rhs->returnType)
    return true;
  else if(lhs->returnType > rhs->returnType)
    return false;
  if(lhs->owner < rhs->owner)
    return true;
  else if(lhs->owner > rhs->owner)
    return false;
  return lexicographical_compare(
      lhs->argTypes.begin(), lhs->argTypes.end(),
      rhs->argTypes.begin(), rhs->argTypes.end());
}

bool TType::canConvert(Type* other)
{
  //other can convert to TType if other implements this trait
  TraitScope* ts = dynamic_cast<TraitScope*>(scope);
  if(!ts || !ts->trait)
  {
    INTERNAL_ERROR;
  }
  return other->implementsTrait(ts->trait);
}

bool TType::canConvert(Expression* other)
{
  return other->type && canConvert(other->type);
}

} //namespace TypeSystem

