#include "TypeSystem.hpp"
#include "Variable.hpp"
#include "Expression.hpp"
#include "Subroutine.hpp"
#include <algorithm>

using std::sort;

/***********************/
/* Type and subclasses */
/***********************/

extern Module* global;

vector<Type*> primitives;
map<string, Type*> primNames;

void createBuiltinTypes()
{
  primitives.resize(14);
  primitives[Prim::BOOL] = new BoolType;
  primitives[Prim::CHAR] = new CharType;
  primitives[Prim::BYTE] = new IntegerType("byte", 1, true);
  primitives[Prim::UBYTE] = new IntegerType("ubyte", 1, false);
  primitives[Prim::SHORT] = new IntegerType("short", 2, true);
  primitives[Prim::USHORT] = new IntegerType("ushort", 2, false);
  primitives[Prim::INT] = new IntegerType("int", 4, true);
  primitives[Prim::UINT] = new IntegerType("uint", 4, false);
  primitives[Prim::LONG] = new IntegerType("long", 8, true);
  primitives[Prim::ULONG] = new IntegerType("ulong", 8, false);
  primitives[Prim::FLOAT] = new FloatType("float", 4);
  primitives[Prim::DOUBLE] = new FloatType("double", 8);
  primitives[Prim::VOID] = new SimpleType("void");
  primitives[Prim::ERROR] = new SimpleType("error");
  primNames["bool"] = primitives[Prim::BOOL];
  primNames["char"] = primitives[Prim::CHAR];
  primNames["byte"] = primitives[Prim::BYTE];
  primNames["ubyte"] = primitives[Prim::UBYTE];
  primNames["short"] = primitives[Prim::SHORT];
  primNames["ushort"] = primitives[Prim::USHORT];
  primNames["int"] = primitives[Prim::INT];
  primNames["uint"] = primitives[Prim::UINT];
  primNames["long"] = primitives[Prim::LONG];
  primNames["ulong"] = primitives[Prim::ULONG];
  primNames["float"] = primitives[Prim::FLOAT];
  primNames["double"] = primitives[Prim::DOUBLE];
  primNames["void"] = primitives[Prim::VOID];
  //string is a builtin alias for char[] (but not a primitive)
  Scope* glob = global->scope;
  auto stringType = new AliasType("string",
      getArrayType(primitives[Prim::CHAR], 1), glob);
  stringType->resolve();
  glob->addName(stringType);
  glob->addName(new AliasType("i8", primitives[Prim::BYTE], glob));
  glob->addName(new AliasType("u8", primitives[Prim::UBYTE], glob));
  glob->addName(new AliasType("i16", primitives[Prim::SHORT], glob));
  glob->addName(new AliasType("u16", primitives[Prim::USHORT], glob));
  glob->addName(new AliasType("i32", primitives[Prim::INT], glob));
  glob->addName(new AliasType("u32", primitives[Prim::UINT], glob));
  glob->addName(new AliasType("i64", primitives[Prim::LONG], glob));
  glob->addName(new AliasType("u64", primitives[Prim::ULONG], glob));
  glob->addName(new AliasType("f32", primitives[Prim::FLOAT], glob));
  glob->addName(new AliasType("f64", primitives[Prim::DOUBLE], glob));
}

Type* getArrayType(Type* elem, int ndims)
{
  resolveType(elem);
  if(ndims == 0)
    return elem;
  auto a = new ArrayType(elem, ndims);
  a->setLocation(elem);
  a->resolve();
  return a;
}

Type* getTupleType(vector<Type*>& members)
{
  for(auto& mem : members)
    resolveType(mem);
  if(members.size() == 1)
    return members[0];
  TupleType* t = new TupleType(members);
  t->resolve();
  return t;
}

Type* getUnionType(vector<Type*>& options)
{
  for(auto& op : options)
    resolveType(op);
  //only one option: union of one thing is just that thing
  if(options.size() == 1)
    return options[0];
  UnionType* u = new UnionType(options);
  u->resolve();
  return u;
}

Type* getMapType(Type* key, Type* value)
{
  resolveType(key);
  resolveType(value);
  MapType* mt = new MapType(key, value);
  mt->resolve();
  return mt;
}

Type* getSubroutineType(StructType* owner, bool pure, Type* retType, vector<Type*>& argTypes)
{
  CallableType* ct = nullptr;
  if(owner)
  {
    ct = new CallableType(pure, owner, retType, argTypes);
  }
  else
  {
    ct = new CallableType(pure, retType, argTypes);
  }
  ct->resolve();
  return ct;
}

Type* promote(Type* lhs, Type* rhs)
{
  INTERNAL_ASSERT(lhs->isNumber() && rhs->isNumber());
  if(typesSame(lhs, rhs))
  {
    return lhs;
  }
  //get type of result as the "most promoted" of lhs and rhs
  //double > float, float > integers, signed > unsigned, wider integer > narrower integer
  if(lhs->isInteger() && rhs->isInteger())
  {
    auto lhsInt = dynamic_cast<IntegerType*>(lhs);
    auto rhsInt = dynamic_cast<IntegerType*>(rhs);
    int size = std::max(lhsInt->size, rhsInt->size);
   bool isSigned = lhsInt->isSigned || rhsInt->isSigned;
    //to combine signed and unsigned of same size, expand to next size if not already 8 bytes
    if(lhsInt->size == rhsInt->size && lhsInt->isSigned != rhsInt->isSigned && size != 8)
    {
      size *= 2;
    }
    //now look up the integer type with given size and signedness
    return getIntegerType(size, isSigned);
  }
  else if(lhs->isInteger())
  {
    //rhs is floating point, so use that
    return rhs;
  }
  else if(rhs->isInteger())
  {
    return lhs;
  }
  else
  {
    //both floats, so pick the bigger one
    auto lhsFloat = dynamic_cast<FloatType*>(lhs);
    auto rhsFloat = dynamic_cast<FloatType*>(rhs);
    if(lhsFloat->size >= rhsFloat->size)
    {
      return lhs;
    }
    else
    {
      return rhs;
    }
  }
  //unreachable
  return nullptr;
}

Type* maybe(Type* t)
{
  vector<Type*> options;
  options.push_back(t);
  options.push_back(primitives[Prim::VOID]);
  auto ut = getUnionType(options);
  ut->setLocation(t);
  return ut;
}

IntegerType* getIntegerType(int bytes, bool isSigned)
{
  switch(bytes)
  {
    case 1:
      if(isSigned)  return (IntegerType*) primitives[Prim::BYTE];
      else          return (IntegerType*) primitives[Prim::UBYTE];
    case 2:
      if(isSigned)  return (IntegerType*) primitives[Prim::SHORT];
      else          return (IntegerType*) primitives[Prim::USHORT];
    case 4:
      if(isSigned)  return (IntegerType*) primitives[Prim::INT];
      else          return (IntegerType*) primitives[Prim::UINT];
    case 8:
      if(isSigned)  return (IntegerType*) primitives[Prim::LONG];
      else          return (IntegerType*) primitives[Prim::ULONG];
    default: INTERNAL_ERROR;
  }
  return nullptr;
}

/***************/
/* Struct Type */
/***************/

StructType::StructType(string n, Scope* enclosingScope)
{
  //structs.push_back(this);
  this->name = n;
  scope = new Scope(enclosingScope, this);
}

void StructType::resolveImpl()
{
  resolved = true;
  //resolve member types first
  for(Variable* mem : members)
  {
    resolveType(mem->type);
    if(!mem->type->resolved)
    {
      resolved = false;
      return;
    }
  }
  //with all member types resolved,
  //can make sure the struct is finite-sized
  for(auto mem : members)
  {
    set<Type*> deps;
    mem->type->dependencies(deps);
    if(deps.find(this) != deps.end())
      errMsgLoc(this, "struct " + name + " contains itself.");
  }
  resolved = false;
  //attempt to resolve all member variables
  for(Variable* mem : members)
  {
    mem->resolve();
    if(!mem->resolved)
    {
      return;
    }
  }
  //all members must now be resolved (including types)
  //all members have been resolved, which means that
  //all member types (including structs) are fully resolved
  //so can now form the interface for this
  //do in reverse priority order so that names are
  //overwritten with higher priority automatically
  for(int i = members.size() - 1; i >= 0; i--)
  {
    if(composed[i])
    {
      auto memStruct = dynamic_cast<StructType*>(members[i]->type);
      if(!memStruct)
      {
        errMsgLoc(members[i], "composition only works on struct types");
      }
      memStruct->resolve();
      //add everything in memStruct's interface to this interface
      for(auto& ifaceKV : memStruct->interface)
      {
        interface[ifaceKV.first] = ifaceKV.second;
      }
    }
  }
  //Build the interface
  //An unresolved member subroutine still knows which struct it
  //belongs to, so it's not necessary to resolve any part of the subroutine here
  for(auto& scopeName : scope->names)
  {
    switch(scopeName.second.kind)
    {
      case Name::SUBROUTINE:
        {
          Subroutine* subr = (Subroutine*) scopeName.second.item;
          //ownerStruct is populated by ctor (before resolve)
          //so it's safe to access here
          if(subr->type->ownerStruct == this)
          {
            interface[subr->name] = IfaceMember(nullptr, subr);
          }
          break;
        }
      case Name::VARIABLE:
        {
          Variable* var = (Variable*) scopeName.second.item;
          //var (member) type can't contain its owner type,
          //so this shouldn't cause a circular dependency
          var->resolve();
          auto ct = dynamic_cast<CallableType*>(var->type);
          if(ct && ct->ownerStruct == this)
          {
            interface[var->name] = IfaceMember(nullptr, var);
          }
          break;
        }
      default:;
    }
  }
  resolved = true;
  //now, it's safe to resolve all members
  scope->resolveAll();
}

//direct conversion requires other to be the same type
bool StructType::canConvert(Type* other)
{
  other = canonicalize(other);
  StructType* otherStruct = dynamic_cast<StructType*>(other);
  TupleType* otherTuple = dynamic_cast<TupleType*>(other);
  if(otherStruct)
  {
    //test memberwise conversion
    if(members.size() != otherStruct->members.size())
      return false;
    for(size_t i = 0; i < members.size(); i++)
    {
      if(!members[i]->type->canConvert(otherStruct->members[i]->type))
        return false;
    }
    return true;
  }
  else if(otherTuple)
  {
    if(members.size() != otherTuple->members.size())
      return false;
    for(size_t i = 0; i < members.size(); i++)
    {
      if(!members[i]->type->canConvert(otherTuple->members[i]))
        return false;
    }
    return true;
  }
  else if(members.size() == 1)
  {
    return members[0]->type->canConvert(other);
  }
  return false;
}

Expression* StructType::getDefaultValue()
{
  vector<Expression*> vals;
  for(size_t i = 0; i < members.size(); i++)
  {
    vals.push_back(members[i]->type->getDefaultValue());
  }
  auto cl = new CompoundLiteral(vals);
  cl->resolve();
  return cl;
}

void StructType::dependencies(set<Type*>& types)
{
  INTERNAL_ASSERT(resolved);
  types.insert(this);
  if(types.find(this) == types.end())
  {
    for(auto mem : members)
    {
      mem->type->dependencies(types);
    }
  }
}

/**************/
/* Union Type */
/**************/

UnionType::UnionType(vector<Type*> types)
{
  options = types;
  defaultVal = nullptr;
}

void UnionType::resolveImpl()
{
  //union type is allowed to have itself as a member,
  //so for the purposes of resolution need to assume this
  //union can be resolved (in order to avoid false circular dependency)
  resolved = true;
  for(size_t i = 0; i < options.size(); i++)
  {
    resolveType(options[i]);
    if(!options[i]->resolved)
    {
      resolved = false;
      return;
    }
    for(size_t j = 0; j < i; j++)
    {
      if(typesSame(options[i], options[j]))
      {
        errMsgLoc(this, "union has duplicated type " + options[i]->getName());
      }
    }
  }
  //fully resolved: now determine whether this is recursive
  recursive = false;
  for(size_t i = 0; i < options.size(); i++)
  {
    set<Type*> opDeps;
    options[i]->dependencies(opDeps);
    if(opDeps.find(this) != opDeps.end())
    {
      //option i does not contain this union, so it's a suitable
      //default value
      recursive = true;
      break;
    }
  }
  //finally, can safely precompute hashes of each option type
  for(auto op : options)
    optionHashes.push_back(op->hash());
}

bool UnionType::canConvert(Type* other)
{
  other = canonicalize(other);
  if(typesSame(other, this))
  {
    return true;
  }
  for(auto op : options)
  {
    if(op->canConvert(other))
      return true;
  }
  return false;
}

void UnionType::dependencies(set<Type*>& types)
{
  INTERNAL_ASSERT(resolved);
  if(types.find(this) == types.end())
  {
    types.insert(this);
    //dependencies to add is the intersection of deps of all members
    map<Type*, int> memIntersect;
    for(auto op : options)
    {
      set<Type*> opDeps;
      opDeps.insert(op);
      op->dependencies(opDeps);
      for(auto opDep : opDeps)
      {
        auto it = memIntersect.find(opDep);
        if(it == memIntersect.end())
          memIntersect[opDep] = 1;
        else
          it->second++;
      }
    }
    //True dependencies are those that appear in every option
    for(auto it : memIntersect)
    {
      if(it.second == (int) options.size())
        types.insert(it.first);
    }
  }
}

string UnionType::getName() const
{
  string name = "(";
  name += options[0]->getName();
  for(size_t i = 1; i < options.size(); i++)
  {
    name += " | ";
    name += options[i]->getName();
  }
  name += ")";
  return name;
}

Expression* UnionType::getDefaultValue()
{
  INTERNAL_ASSERT(resolved);
  if(!defaultVal)
  {
    int defaultType = -1;
    for(size_t i = 0; i < options.size(); i++)
    {
      set<Type*> opDeps;
      options[i]->dependencies(opDeps);
      if(opDeps.find(this) == opDeps.end())
      {
        //option i does not contain this union, so it's a suitable
        //default value
        defaultType = i;
        break;
      }
    }
    INTERNAL_ASSERT(defaultType >= 0);
    defaultVal = new UnionConstant(
        options[defaultType]->getDefaultValue(),
        options[defaultType], this);
    defaultVal->resolve();
  }
  return defaultVal;
}

int UnionType::getTypeIndex(Type* t)
{
  size_t thash = t->hash();
  for(size_t i = 0; i < options.size(); i++)
  {
    if(optionHashes[i] == thash && typesSame(options[i], t))
      return i;
  }
  INTERNAL_ERROR;
  return -1;
}

/**************/
/* Array Type */
/**************/

ArrayType::ArrayType(Type* elemType, int ndims)
{
  INTERNAL_ASSERT(elemType)
  INTERNAL_ASSERT(ndims > 0)
  this->dims = ndims;
  this->elem = elemType;
  //If 1-dimensional, subtype is just elem
  //Otherwise is array with one fewer dimension
  subtype = (ndims == 1) ? elem : getArrayType(elemType, dims - 1);
}

void ArrayType::resolveImpl()
{
  resolveType(elem);
  set<Type*> deps;
  elem->dependencies(deps);
  if(deps.find(this) != deps.end())
  {
    errMsg("array type " + getName() + " contains itself");
  }
  resolved = true;
}

bool ArrayType::canConvert(Type* other)
{
  other = canonicalize(other);
  auto otherArray = dynamic_cast<ArrayType*>(other);
  auto otherTuple = dynamic_cast<TupleType*>(other);
  auto otherStruct = dynamic_cast<StructType*>(other);
  if(otherArray)
  {
    return subtype->canConvert(otherArray->subtype);
  }
  else if(otherTuple)
  {
    for(auto mem : otherTuple->members)
    {
      if(!subtype->canConvert(mem))
        return false;
    }
    return true;
  }
  else if(otherStruct)
  {
    for(auto mem : otherStruct->members)
    {
      if(!subtype->canConvert(mem->type))
        return false;
    }
    return true;
  }
  return false;
}

Expression* ArrayType::getDefaultValue()
{
  vector<Expression*> empty;
  CompoundLiteral* cl = new CompoundLiteral(empty);
  cl->type = this;
  cl->resolved = true;
  return cl;
}

void ArrayType::dependencies(set<Type*>& types)
{
  INTERNAL_ASSERT(resolved);
  types.insert(this);
  elem->dependencies(types);
}

/**************/
/* Tuple Type */
/**************/

TupleType::TupleType(vector<Type*> mems)
{
  members = mems;
}

void TupleType::resolveImpl()
{
  for(Type*& mem : members)
  {
    resolveType(mem);
  }
  resolved = true;
  for(auto mem : members)
  {
    set<Type*> deps;
    mem->dependencies(deps);
    if(deps.find(this) != deps.end())
      errMsgLoc(this, "tuple contains itself.");
  }
}

bool TupleType::canConvert(Type* other)
{
  other = canonicalize(other);
  TupleType* otherTuple = dynamic_cast<TupleType*>(other);
  StructType* otherStruct = dynamic_cast<StructType*>(other);
  if(otherStruct)
  {
    //test memberwise conversion
    if(members.size() != otherStruct->members.size())
      return false;
    for(size_t i = 0; i < members.size(); i++)
    {
      if(!members[i]->canConvert(otherStruct->members[i]->type))
        return false;
    }
    return true;
  }
  else if(otherTuple)
  {
    if(members.size() != otherTuple->members.size())
      return false;
    for(size_t i = 0; i < members.size(); i++)
    {
      if(!members[i]->canConvert(otherTuple->members[i]))
        return false;
    }
    return true;
  }
  return members.size() == 1 && members[0]->canConvert(other);
}

Expression* TupleType::getDefaultValue()
{
  vector<Expression*> vals;
  for(size_t i = 0; i < members.size(); i++)
  {
    vals.push_back(members[i]->getDefaultValue());
  }
  CompoundLiteral* cl = new CompoundLiteral(vals);
  cl->resolved = true;
  cl->type = this;
  return cl;
}

void TupleType::dependencies(set<Type*>& types)
{
  INTERNAL_ASSERT(resolved);
  if(types.find(this) == types.end())
  {
    types.insert(this);
    for(auto m : members)
    {
      types.insert(m);
      m->dependencies(types);
    }
  }
}

/************/
/* Map Type */
/************/

MapType::MapType(Type* k, Type* v) : key(k), value(v) {}

void MapType::resolveImpl()
{
  resolveType(key);
  resolveType(value);
  resolved = true;
}

bool MapType::canConvert(Type* other)
{
  other = canonicalize(other);
  //Maps can convert to this if keys/values can convert
  //Arrays can also convert to this if key of this is integer
  auto otherMap = dynamic_cast<MapType*>(other);
  auto otherArray = dynamic_cast<ArrayType*>(other);
  if(otherMap)
  {
    return key->canConvert(otherMap->key) &&
      value->canConvert(otherMap->value);
  }
  if(otherArray)
  {
    TupleType* subtypeTuple = dynamic_cast<TupleType*>(otherArray->subtype);
    //must be "(k, v)[]" where k convertible to key and v convertible to value
    return subtypeTuple &&
      subtypeTuple->members.size() == 2 &&
      key->canConvert(subtypeTuple->members[0]) &&
      !value->canConvert(subtypeTuple->members[1]);
  }
  return false;
}

void MapType::dependencies(set<Type*>& types)
{
  key->dependencies(types);
  value->dependencies(types);
}

/**************/
/* Alias Type */
/**************/

AliasType::AliasType(string alias, Type* underlying, Scope* s)
{
  name = alias;
  actual = underlying;
  scope = s;
}

void AliasType::resolveImpl()
{
  //AliasType can legally refer to itself through a union,
  //so pretend it's resolved during the resolution. Don't
  //report false circular dependency error.
  resolved = true;
  resolveType(actual);
  //AliasType resolution fails if and only if the
  //underlying type failed to resolve
  resolved = actual->resolved;
}

/*************/
/* Enum Type */
/*************/

EnumType::EnumType(Scope* enclosingScope)
{
  //"scope" encloses the enum constants
  scope = new Scope(enclosingScope, this);
}

void EnumType::resolveImpl()
{
  //Decide what integer type will represent the enum
  //Prefer signed, and then prefer smaller widths
  bool canUseS = true;
  bool canUseU = true;
  for(auto ec : values)
  {
    if(!ec->fitsS64)
      canUseS = false;
    if(!ec->fitsU64)
      canUseU = false;
  }
  if(!canUseS && !canUseU)
  {
    errMsgLoc(this, "neither long nor ulong can represent all values in enum");
  }
  //Try different integer widths until all values fit
  for(int width = 1; width <= 8; width *= 2)
  {
    underlying = getIntegerType(width, canUseS);
    if(canUseS)
    {
      for(auto ec : values)
      {
        if(ec->sval < underlying->minSignedVal() ||
            ec->sval > underlying->maxSignedVal())
        {
          underlying = nullptr;
          break;
        }
      }
    }
    else
    {
      for(auto ec : values)
      {
        if(ec->uval > underlying->maxUnsignedVal())
        {
          underlying = nullptr;
          break;
        }
      }
    }
    if(underlying)
    {
      //found the smallest type that works, done
      break;
    }
  }
  resolved = true;
}

void EnumType::addAutomaticValue(string n, Node* location)
{
  uint64_t uval = 0;
  if(!values.back()->fitsU64)
  {
    //previously added value was negative
    for(int64_t sval = values.back()->sval + 1; sval < 0; sval++)
    {
      //check if sval is already in the enum
      bool alreadyInEnum = false;
      for(auto existing : values)
      {
        if(!existing->fitsU64 && existing->sval == sval)
        {
          alreadyInEnum = true;
          break;
        }
      }
      if(!alreadyInEnum)
      {
        addNegativeValue(n, sval, location);
        return;
      }
    }
    //fall through: start trying unsigned values to insert at 0
  }
  else if(!values.empty())
  {
    uval = values.back()->uval + 1;
  }
  //otherwise, start searching at uval = 0
  for(;; uval++)
  {
    bool alreadyInEnum = false;
    for(auto existing : values)
    {
      if(existing->fitsU64 && existing->uval == uval)
      {
        alreadyInEnum = true;
        break;
      }
    }
    if(!alreadyInEnum)
    {
      addPositiveValue(n, uval, location);
      return;
    }
  }
}

void EnumType::addPositiveValue(string n, uint64_t uval, Node* location)
{
  //uval must not already be in the enum
  for(auto existing : values)
  {
    if(existing->fitsU64 && existing->uval == uval)
    {
      errMsgLoc(this, "enum value " << n << " duplicates value of " << existing->name);
    }
  }
  EnumConstant* newValue = new EnumConstant(n, uval);
  newValue->setLocation(location);
  newValue->et = this;
  scope->addName(newValue);
  values.push_back(newValue);
}

void EnumType::addNegativeValue(string n, int64_t sval, Node* location)
{
  for(auto existing : values)
  {
    if(!existing->fitsU64 && existing->sval == sval)
    {
      errMsgLoc(this, "enum value " << n << " duplicates value of " << existing->name);
    }
  }
  EnumConstant* newValue = new EnumConstant(n, sval);
  newValue->setLocation(location);
  newValue->et = this;
  scope->addName(newValue);
  values.push_back(newValue);
}

bool EnumType::canConvert(Type* other)
{
  other = canonicalize(other);
  return other->isInteger();
}

/****************/
/* Integer Type */
/****************/

IntegerType::IntegerType(string typeName, int sz, bool sign)
{
  name = typeName;
  size = sz;
  isSigned = sign;
  resolved = true;
}

uint64_t IntegerType::maxUnsignedVal()
{
  INTERNAL_ASSERT(!isSigned);
  switch(size)
  {
    case 1:
      return numeric_limits<uint8_t>::max();
    case 2:
      return numeric_limits<uint16_t>::max();
    case 4:
      return numeric_limits<uint32_t>::max();
    default:;
  }
  return numeric_limits<uint64_t>::max();
}

int64_t IntegerType::minSignedVal()
{
  INTERNAL_ASSERT(isSigned);
  switch(size)
  {
    case 1:
      return numeric_limits<int8_t>::min();
    case 2:
      return numeric_limits<int16_t>::min();
    case 4:
      return numeric_limits<int32_t>::min();
    default:;
  }
  return numeric_limits<int64_t>::min();
}

int64_t IntegerType::maxSignedVal()
{
  INTERNAL_ASSERT(isSigned);
  switch(size)
  {
    case 1:
      return numeric_limits<int8_t>::max();
    case 2:
      return numeric_limits<int16_t>::max();
    case 4:
      return numeric_limits<int32_t>::max();
    default:;
  }
  return numeric_limits<int64_t>::max();
}

Expression* IntegerType::getDefaultValue()
{
  IntConstant* ic = new IntConstant;
  ic->type = this;
  return ic;
}

bool IntegerType::canConvert(Type* other)
{
  return other->isNumber();
}

/**************/
/* Float Type */
/**************/

FloatType::FloatType(string typeName, int sz)
{
  name = typeName;
  size = sz;
  resolved = true;
}

bool FloatType::canConvert(Type* other)
{
  return other->isNumber();
}

Expression* FloatType::getDefaultValue()
{
  FloatConstant* fc = new FloatConstant();
  fc->type = this;
  return fc;
}

/*************/
/* Char Type */
/*************/

bool CharType::canConvert(Type* other)
{
  return other->isInteger();
}

Expression* CharType::getDefaultValue()
{
  return new CharConstant('\0');
}

/*************/
/* Bool Type */
/*************/

bool BoolType::canConvert(Type* other)
{
  return other->isBool();
}

Expression* BoolType::getDefaultValue()
{
  return new BoolConstant(false);
}

/*****************/
/* Callable Type */
/*****************/

CallableType::CallableType(bool isPure, Type* retType, vector<Type*>& params)
{
  pure = isPure;
  returnType = retType;
  paramTypes = params;
  ownerStruct = nullptr;
}

CallableType::CallableType(bool isPure, StructType* owner, Type* retType, vector<Type*>& params)
{
  pure = isPure;
  returnType = retType;
  paramTypes = params;
  ownerStruct = owner;
}

void CallableType::resolveImpl()
{
  //CallableType is allowed to have itself as a return or argument type,
  //so temporarily pretend it is resolved to avoid circular dependency error
  resolved = true;
  resolveType(returnType);
  for(Type*& param : paramTypes)
  {
    resolveType(param);
  }
  //just leave resolved = true
}

string CallableType::getName() const
{
  Oss oss;
  if(pure)
    oss << "func ";
  else
    oss << "proc ";
  oss << returnType->getName();
  oss << "(";
  for(size_t i = 0; i < paramTypes.size(); i++)
  {
    oss << paramTypes[i]->getName();
    if(i < paramTypes.size() - 1)
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
bool CallableType::canConvert(Type* other)
{
  //Only CallableTypes are convertible to other CallableTypes
  auto ct = dynamic_cast<CallableType*>(other);
  if(!ct)
    return false;
  if(ownerStruct != ct->ownerStruct)
  {
    return false;
  }
  if(pure && !ct->pure)
    return false;
  //check that arguments are exactly the same
  //doing at end because more expensive test
  if(paramTypes.size() != ct->paramTypes.size())
    return false;
  for(size_t i = 0; i < paramTypes.size(); i++)
  {
    if(!typesSame(paramTypes[i], ct->paramTypes[i]))
      return false;
  }
  return true;
}

SimpleType::SimpleType(string n)
{
  resolved = true;
  name = n;
  val = new SimpleConstant(this);
}

Expression* SimpleType::getDefaultValue()
{
  return val;
}

ExprType::ExprType(Expression* e)
{
  expr = e;
}

void ExprType::resolveImpl()
{
  //should never get here,
  //ExprType must be replaced by another type in resolveType()
  INTERNAL_ERROR;
}

ElemExprType::ElemExprType(Expression* a) : arr(a) {}

void ElemExprType::resolveImpl()
{
  //should never get here,
  //ElemExprType must be replaced by another type in resolveType()
  INTERNAL_ERROR;
}

void resolveType(Type*& t)
{
  INTERNAL_ASSERT(t);
  Node* loc = t;
  if(t->resolved)
  {
    //nothing to do
    return;
  }
  if(UnresolvedType* unres = dynamic_cast<UnresolvedType*>(t))
  {
    if(unres->t.is<Prim::PrimType>())
    {
      t = primitives[unres->t.get<Prim::PrimType>()];
    }
    else if(unres->t.is<Member*>())
    {
      auto mem = unres->t.get<Member*>();
      Name found = unres->scope->findName(mem);
      //name wasn't found
      //if this is the last chance to resolve type, is an error
      if(!found.item)
      {
        errMsgLoc(unres, "unknown type " << *mem);
      }
      switch(found.kind)
      {
        case Name::STRUCT:
          t = (StructType*) found.item;
          break;
        case Name::SIMPLE_TYPE:
          t = (SimpleType*) found.item;
          break;
        case Name::ENUM:
          t = (EnumType*) found.item;
          break;
        case Name::TYPEDEF:
          t = (AliasType*) found.item;
          break;
        default:
          errMsgLoc(unres, "name " << mem << " does not refer to a type");
      }
      t->resolve();
    }
    else if(unres->t.is<UnresolvedType::Tuple>())
    {
      auto& tupleList = unres->t.get<UnresolvedType::Tuple>();
      t = new TupleType(tupleList.members);
      t->resolve();
    }
    else if(unres->t.is<UnresolvedType::Union>())
    {
      //first, resolve the elements of the union
      auto& unionList = unres->t.get<UnresolvedType::Union>();
      t = new UnionType(unionList.members);
      t->resolve();
    }
    else if(unres->t.is<UnresolvedType::Map>())
    {
      auto& kv = unres->t.get<UnresolvedType::Map>();
      t = new MapType(kv.key, kv.value);
      t->resolve();
    }
    else if(unres->t.is<UnresolvedType::Callable>())
    {
      //walk up scope tree to see if in a non-static context
      auto ownerStruct = unres->scope->getStructContext();
      auto& ct = unres->t.get<UnresolvedType::Callable>();
      bool isStatic = ct.isStatic || !ownerStruct;
      bool allResolved = true;
      resolveType(ct.returnType);
      if(!ct.returnType->resolved)
      {
        allResolved = false;
      }
      for(auto& param : ct.params)
      {
        resolveType(param);
        if(!param->resolved)
        {
          allResolved = false;
        }
      }
      if(allResolved)
      {
        if(isStatic)
        {
          t = new CallableType(ct.pure, ct.returnType, ct.params);
        }
        else
        {
          t = new CallableType(ct.pure, ownerStruct,
              ct.returnType, ct.params);
        }
      }
      t->resolve();
    }
    if(!t->resolved)
    {
      //can't apply array dimensions, so return early
      return;
    }
    if(unres->arrayDims > 0)
    {
      t = new ArrayType(t, unres->arrayDims);
      t->resolve();
    }
  }
  else if(ExprType* et = dynamic_cast<ExprType*>(t))
  {
    resolveExpr(et->expr);
    if(et->expr->resolved)
      t = et->expr->type;
  }
  else if(ElemExprType* eet = dynamic_cast<ElemExprType*>(t))
  {
    resolveExpr(eet->arr);
    ArrayType* arrType = dynamic_cast<ArrayType*>(eet->arr->type);
    if(!arrType)
    {
      //arr's type is already singular, so use that
      t = eet->arr->type;
    }
    else
    {
      t = arrType->elem;
    }
  }
  else
  {
    t->resolve();
  }
  t->setLocation(loc);
  INTERNAL_ASSERT(t->resolved);
}

typedef pair<const Type*, const Type*> TypePair;

//This function implements operator==(Type*, Type*)
//Needs "assumptions" list so that recursive type
//comparison terminates.
static bool typesSameImpl(const Type* t1, const Type* t2,
    set<TypePair>& assume)
{
  if(t1 == t2)
    return true;
  //note: == is commutative so check for both (t1, t2) and (t2, t1)
  if(assume.find(TypePair(t1, t2)) != assume.end() ||
      assume.find(TypePair(t2, t1)) != assume.end())
    return true;
  assume.insert(TypePair(t1, t2));
  //first, canonicalize aliases
  while(auto at = dynamic_cast<const AliasType*>(t1))
  {
    t1 = at->actual;
  }
  while(auto at = dynamic_cast<const AliasType*>(t2))
  {
    t2 = at->actual;
  }
  if(auto a1 = dynamic_cast<const ArrayType*>(t1))
  {
    auto a2 = dynamic_cast<const ArrayType*>(t2);
    if(!a2)
      return false;
    return typesSameImpl(a1->subtype, a2->subtype, assume);
  }
  else if(auto tt1 = dynamic_cast<const TupleType*>(t1))
  {
    auto tt2 = dynamic_cast<const TupleType*>(t2);
    if(!tt2)
      return false;
    if(tt1->members.size() != tt2->members.size())
      return false;
    for(size_t i = 0; i < tt1->members.size(); i++)
    {
      if(!typesSameImpl(tt1->members[i], tt2->members[i], assume))
        return false;
    }
    return true;
  }
  else if(auto u1 = dynamic_cast<const UnionType*>(t1))
  {
    auto u2 = dynamic_cast<const UnionType*>(t2);
    if(!u2)
      return false;
    if(u1->options.size() != u2->options.size())
      return false;
    for(size_t i = 0; i < u1->options.size(); i++)
    {
      if(!typesSameImpl(u1->options[i], u2->options[i], assume))
        return false;
    }
    return true;
  }
  else if(auto m1 = dynamic_cast<const MapType*>(t1))
  {
    auto m2 = dynamic_cast<const MapType*>(t2);
    if(!m2)
      return false;
    if(!typesSameImpl(m1->key, m2->key, assume))
      return false;
    if(!typesSameImpl(m1->value, m2->value, assume))
      return false;
    return true;
  }
  else if(auto c1 = dynamic_cast<const CallableType*>(t1))
  {
    auto c2 = dynamic_cast<const CallableType*>(t2);
    if(!c2)
      return false;
    if(c1->pure != c2->pure)
      return false;
    if(c1->ownerStruct != c2->ownerStruct)
      return false;
    if(c1->paramTypes.size() != c2->paramTypes.size())
      return false;
    if(!typesSameImpl(c1->returnType, c2->returnType, assume))
      return false;
    for(size_t i = 0; i < c1->paramTypes.size(); i++)
    {
      if(!typesSameImpl(c1->paramTypes[i], c2->paramTypes[i], assume))
        return false;
    }
    return true;
  }
  //There should be no need to compare other types
  return false;
}

bool typesSame(const Type* t1, const Type* t2)
{
  set<TypePair> assume;
  return typesSameImpl(t1, t2, assume);
}

Type* canonicalize(Type* t)
{
  while(auto at = dynamic_cast<AliasType*>(t))
  {
    t = at->actual;
  }
  return t;
}

