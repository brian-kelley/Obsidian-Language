#include "TypeSystem.hpp"
#include "Variable.hpp"
#include "Expression.hpp"
#include "Subroutine.hpp"

/***********************/
/* Type and subclasses */
/***********************/

extern Module* global;

vector<Type*> primitives;

map<string, Type*> primNames;
vector<StructType*> structs;
set<ArrayType*, ArrayCompare> arrays;
set<TupleType*, TupleCompare> tuples;
set<UnionType*, UnionCompare> unions;
set<MapType*, MapCompare> maps;
set<CallableType*, CallableCompare> callables;
set<EnumType*> enums;

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
  primitives[Prim::VOID] = new VoidType;
  primitives[Prim::ERROR] = new ErrorType;
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
  primNames["Error"] = primitives[Prim::ERROR];
  //string is a builtin alias for char[] (not a primitive)
  Scope* glob = global->scope;
  glob->addName(new AliasType(
        "string", getArrayType(primitives[Prim::CHAR], 1), glob));
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
  if(!elem)
    return nullptr;
  if(ndims == 0)
    return elem;
  ArrayType* at = nullptr;
  if(auto elemArray = dynamic_cast<ArrayType*>(elem))
  {
    at = new ArrayType(elemArray->elem, elemArray->dims + ndims);
  }
  else
  {
    at = new ArrayType(elem, ndims);
  }
  auto it = arrays.find(at);
  if(it == arrays.end())
  {
    arrays.insert(at);
    return at;
  }
  delete at;
  return *it;
}

Type* getTupleType(vector<Type*>& members)
{
  for(auto mem : members)
  {
    if(mem == nullptr)
      return nullptr;
  }
  TupleType* newTuple = new TupleType(members);
  auto it = tuples.find(newTuple);
  if(it != tuples.end())
  {
    delete newTuple;
    return *it;
  }
  //new tuple type, add to set
  tuples.insert(newTuple);
  return newTuple;
}

Type* getUnionType(vector<Type*>& options)
{
  for(auto mem : options)
  {
    if(mem == nullptr)
      return nullptr;
  }
  //only one option: union of one thing is itself
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

Type* getMapType(Type* key, Type* value)
{
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

Type* getSubroutineType(StructType* owner, bool pure, Type* retType, vector<Type*>& argTypes)
{
  if(retType == nullptr)
    return nullptr;
  for(auto arg : argTypes)
  {
    if(arg == nullptr)
      return nullptr;
  }
  auto ct = new CallableType(pure, owner, retType, argTypes);
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

Type* promote(Type* lhs, Type* rhs)
{
  if(!lhs->isNumber() || !rhs->isNumber())
  {
    return nullptr;
  }
  if(lhs == rhs)
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
  options.push_back(primitives[Prim::ERROR]);
  return getUnionType(options);
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
  structs.push_back(this);
  this->name = n;
  scope = new Scope(enclosingScope, this);
}

void StructType::resolveImpl(bool final)
{
  //attempt to resolve all member variables
  bool allResolved = true;
  for(Variable* mem : members)
  {
    //resolve members requires resolving member types,
    //and calling resolve() on this while already in a resolve
    //call triggers a "circular dependency" error, preventing
    //self-ownership
    mem->resolve(final);
    if(!mem->resolved)
      allResolved = false;
  }
  if(!allResolved)
    return;
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
        errMsgLoc(members[i], "composition requested on non-struct member");
      }
      //add everything in memStruct's interface to this interface
      for(auto& ifaceKV : memStruct->interface)
      {
        interface[ifaceKV.first] = ifaceKV.second;
      }
    }
  }
  if(!scope->resolveAll(final))
    return;
  //then add all the direct methods of this
  //need to search all submodules for subroutines and callable members
  for(auto& scopeName : scope->names)
  {
    switch(scopeName.second.kind)
    {
      case Name::SUBROUTINE:
        {
          Subroutine* subr = (Subroutine*) scopeName.second.item;
          if(subr->type->ownerStruct == this)
          {
            interface[subr->name] = IfaceMember(nullptr, subr);
          }
          break;
        }
      case Name::VARIABLE:
        {
          Variable* var = (Variable*) scopeName.second.item;
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
}

//direct conversion requires other to be the same type
bool StructType::canConvert(Type* other)
{
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
  return false;
}

/**************/
/* Union Type */
/**************/

UnionType::UnionType(vector<Type*> types)
{
  options = types;
  sort(options.begin(), options.end());
}

void UnionType::resolveImpl(bool final)
{
  //union type is allowed to have itself as a member,
  //so for the purposes of resolution need to assume this
  //can be resolved
  resolved = true;
  for(Type*& mem : options)
  {
    resolveType(mem, final);
    if(!mem->resolved)
    {
      resolved = false;
      return;
    }
  }
  setDefault();
}

void UnionType::setDefault()
{
  for(size_t i = 0; i < options.size(); i++)
  {
    //find all the types reachable from this option,
    //then see if this is reachable
    set<Type*> visited;
    stack<Type*> search;
    search.push(options[i]);
    bool reachable = false;
    while(!search.empty())
    {
      Type* process = search.top();
      if(process == this)
      {
        reachable = true;
        break;
      }
      search.pop();
      visited.insert(process);
      if(auto ut = dynamic_cast<UnionType*>(process))
      {
        for(auto op : ut->options)
        {
          if(visited.find(op) == visited.end())
          {
            search.push(op);
          }
        }
      }
      else if(auto st = dynamic_cast<StructType*>(process))
      {
        for(auto mem : st->members)
        {
          if(visited.find(mem->type) == visited.end())
          {
            search.push(mem->type);
          }
        }
      }
      else if(auto at = dynamic_cast<ArrayType*>(process))
      {
        if(visited.find(at->elem) == visited.end())
        {
          search.push(at->elem);
        }
      }
    }
    if(!reachable)
    {
      //OK to use option i as default
      defaultType = i;
      return;
    }
  }
  errMsg("all members of union type " << getName() << " contain the union");
}

bool UnionType::canConvert(Type* other)
{
  //Assignment to a Union will use the first exact
  //type match, then any successful conversion
  for(auto op : options)
  {
    if(op->canConvert(other))
    {
      return true;
    }
  }
  return false;
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
  return lexicographical_compare(lhs->options.begin(), lhs->options.end(),
      rhs->options.begin(), rhs->options.end());
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

void ArrayType::resolveImpl(bool final)
{
  resolveType(elem, final);
  resolved = elem->resolved;
}

bool ArrayType::canConvert(Type* other)
{
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

bool ArrayCompare::operator()(const ArrayType* lhs, const ArrayType* rhs)
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

TupleType::TupleType(vector<Type*> mems)
{
  members = mems;
}

void TupleType::resolveImpl(bool final)
{
  for(Type*& mem : members)
  {
    resolveType(mem, final);
    if(!mem->resolved)
      return;
  }
  resolved = true;
}

bool TupleType::canConvert(Type* other)
{
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

bool TupleCompare::operator()(const TupleType* lhs, const TupleType* rhs)
{
  return lexicographical_compare(lhs->members.begin(), lhs->members.end(),
      rhs->members.begin(), rhs->members.end());
}

/************/
/* Map Type */
/************/

MapType::MapType(Type* k, Type* v) : key(k), value(v) {}

void MapType::resolveImpl(bool final)
{
  resolveType(key, final);
  if(!key->resolved)
    return;
  resolveType(value, final);
  resolved = value->resolved;
}

bool MapType::canConvert(Type* other)
{
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

bool MapCompare::operator()(const MapType* lhs, const MapType* rhs)
{
  return (lhs->key < rhs->key) || (lhs->key == rhs->key && lhs->value < rhs->value);
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

void AliasType::resolveImpl(bool final)
{
  resolveType(actual, final);
  resolved = actual->resolved;
}

bool AliasType::canConvert(Type* other)
{
  return actual->canConvert(other);
}

/*************/
/* Enum Type */
/*************/

EnumType::EnumType(Scope* enclosingScope)
{
  //"scope" encloses the enum constants
  scope = new Scope(enclosingScope, this);
}

void EnumType::resolveImpl(bool final)
{
  //Decide what integer type will represent the enum
  //Prefer signed and then prefer smaller widths
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
    errMsgLoc(this, "neither long nor ulong canrepresent all values in enum");
  }
  bool allFit = true;
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

void EnumType::addAutomaticValue(string name, Node* location)
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
        addNegativeValue(name, sval, location);
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
      addPositiveValue(name, uval, location);
      return;
    }
  }
}

void EnumType::addPositiveValue(string name, uint64_t uval, Node* location)
{
  //uval must not already be in the enum
  for(auto existing : values)
  {
    if(existing->fitsU64 && existing->uval == uval)
    {
      errMsgLoc(this, "enum value " << name << " duplicates value of " << existing->name);
    }
  }
  EnumConstant* newValue = new EnumConstant(name, uval);
  newValue->setLocation(location);
  newValue->et = this;
  scope->addName(newValue);
  values.push_back(newValue);
}

void EnumType::addNegativeValue(string name, int64_t sval, Node* location)
{
  for(auto existing : values)
  {
    if(!existing->fitsU64 && existing->sval == sval)
    {
      errMsgLoc(this, "enum value " << name << " duplicates value of " << existing->name);
    }
  }
  EnumConstant* newValue = new EnumConstant(name, sval);
  newValue->setLocation(location);
  newValue->et = this;
  scope->addName(newValue);
  values.push_back(newValue);
}

bool EnumType::canConvert(Type* other)
{
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

bool IntegerType::canConvert(Type* other)
{
  return other->isEnum() || other->isInteger();
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

bool BoolType::canConvert(Type* other)
{
  return other->isBool();
}

/*************/
/* Void Type */
/*************/

bool VoidType::canConvert(Type* t)
{
  return t->isVoid();
}

/*****************/
/* Callable Type */
/*****************/

CallableType::CallableType(bool isPure, Type* retType, vector<Type*>& args)
{
  pure = isPure;
  returnType = retType;
  argTypes = args;
  ownerStruct = nullptr;
}

CallableType::CallableType(bool isPure, StructType* owner, Type* retType, vector<Type*>& args)
{
  pure = isPure;
  returnType = retType;
  argTypes = args;
  ownerStruct = owner;
}

void CallableType::resolveImpl(bool final)
{
  //CallableType is allowed to have itself as a return or argument type,
  //so temporarily pretend it is resolved to avoid circular dependency error
  resolved = true;
  resolveType(returnType, final);
  if(!returnType->resolved)
  {
    resolved = false;
    return;
  }
  for(Type*& arg : argTypes)
  {
    resolveType(arg, final);
    if(!arg->resolved)
    {
      resolved = false;
      return;
    }
  }
  //just leave resolved = true
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
  if(argTypes != ct->argTypes)
    return false;
  return true;
}

bool CallableCompare::operator()(const CallableType* lhs, const CallableType* rhs)
{
  //an arbitrary way to order all possible callables (is lhs < rhs?)
  if(!lhs->pure && rhs->pure)
    return true;
  if(lhs->returnType < rhs->returnType)
    return true;
  else if(lhs->returnType > rhs->returnType)
    return false;
  if(lhs->ownerStruct < rhs->ownerStruct)
    return true;
  else if(lhs->ownerStruct > rhs->ownerStruct)
    return false;
  return lexicographical_compare(
      lhs->argTypes.begin(), lhs->argTypes.end(),
      rhs->argTypes.begin(), rhs->argTypes.end());
}

ExprType::ExprType(Expression* e)
{
  expr = e;
}

void ExprType::resolveImpl(bool)
{
  //should never get here,
  //ExprType must be replaced by another type in resolveType()
  INTERNAL_ERROR;
}

ElemExprType::ElemExprType(Expression* a) : arr(a) {}

void ElemExprType::resolveImpl(bool)
{
  //should never get here,
  //ElemExprType must be replaced by another type in resolveType()
  INTERNAL_ERROR;
}

void resolveType(Type*& t, bool final)
{
  if(t->isResolved())
  {
    //nothing to do
    return;
  }
  Type* finalType = nullptr;
  if(UnresolvedType* unres = dynamic_cast<UnresolvedType*>(t))
  {
    if(unres->t.is<Prim::PrimType>())
    {
      finalType = primitives[unres->t.get<Prim::PrimType>()];
    }
    else if(unres->t.is<Member*>())
    {
      auto mem = unres->t.get<Member*>();
      Name found = unres->scope->findName(mem);
      //name wasn't found
      //if this is the last chance to resolve type, is an error
      if(!found.item && final)
      {
        errMsgLoc(unres, "unknown type " << *mem);
      }
      switch(found.kind)
      {
        case Name::STRUCT:
          finalType = (StructType*) found.item;
          break;
        case Name::ENUM:
          finalType = (EnumType*) found.item;
          break;
        case Name::TYPEDEF:
          finalType = ((AliasType*) found.item)->actual;
          break;
        default:
          errMsgLoc(unres, "name " << mem << " does not refer to a type");
      }
    }
    else if(unres->t.is<UnresolvedType::Tuple>())
    {
      auto& tupList = unres->t.get<UnresolvedType::Tuple>();
      //resolve member types individually
      bool allResolved = true;
      for(Type*& mem : tupList.members)
      {
        resolveType(mem, final);
        if(!mem->isResolved())
          allResolved = false;
      }
      if(allResolved)
      {
        finalType = getTupleType(tupList.members);
      }
    }
    else if(unres->t.is<UnresolvedType::Union>())
    {
      auto& unionList = unres->t.get<UnresolvedType::Union>();
      //resolve member types individually
      bool allResolved = true;
      for(Type*& option : unionList.members)
      {
        resolveType(option, final);
        if(!t->isResolved())
          allResolved = false;
      }
      if(allResolved)
      {
        finalType = getUnionType(unionList.members);
      }
    }
    else if(unres->t.is<UnresolvedType::Map>())
    {
      auto& kv = unres->t.get<UnresolvedType::Map>();
      resolveType(kv.key, final);
      resolveType(kv.value, final);
      if(kv.key->isResolved() && kv.value->isResolved())
      {
        finalType = getMapType(kv.key, kv.value);
      }
    }
    else if(unres->t.is<UnresolvedType::Callable>())
    {
      //walk up scope tree to see if in a non-static context
      auto ownerStruct = unres->scope->getStructContext();
      auto& ct = unres->t.get<UnresolvedType::Callable>();
      bool allResolved = true;
      resolveType(ct.returnType, final);
      if(!ct.returnType->isResolved())
      {
        allResolved = false;
      }
      for(auto& param : ct.params)
      {
        resolveType(param, final);
        if(!param->isResolved())
        {
          allResolved = false;
        }
      }
      if(allResolved)
      {
        finalType = getSubroutineType(ownerStruct, ct.pure,
            ct.returnType, ct.params);
      }
    }
    if(!finalType)
    {
      //can't apply array dimensions, so return early
      return;
    }
    //if arrayDims is 0, this is a no-op
    finalType = getArrayType(finalType, unres->arrayDims);
    finalType->finalResolve();
  }
  else if(ExprType* et = dynamic_cast<ExprType*>(t))
  {
    resolveExpr(et->expr, final);
    if(!et->expr->resolved)
      return;
    finalType = et->expr->type;
  }
  else if(ElemExprType* eet = dynamic_cast<ElemExprType*>(t))
  {
    resolveExpr(eet->arr, final);
    if(!eet->arr->resolved)
      return;
    ArrayType* arrType = dynamic_cast<ArrayType*>(eet->arr->type);
    if(!arrType)
    {
      //arr's type is already singular, so use that
      finalType = eet->arr->type;
    }
    else
    {
      finalType = arrType->elem;
    }
  }
  else
  {
    t->resolve(final);
    return;
  }
  //finally, replace unres with finalType
  t = finalType;
}

