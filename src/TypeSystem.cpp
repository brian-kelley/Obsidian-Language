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
  if(auto subArray = dynamic_cast<ArrayType*>(elem))
  {
    ndims += subArray->dims;
    elem = subArray->elem;
  }
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
  if(auto leftEnum = dynamic_cast<EnumType*>(lhs))
    lhs = leftEnum->underlying;
  if(auto rightEnum = dynamic_cast<EnumType*>(rhs))
    rhs = rightEnum->underlying;
  INTERNAL_ASSERT(lhs->isNumber() && rhs->isNumber());
  if(typesSame(lhs, rhs))
  {
    return lhs;
  }
  //get type of result as the "most promoted" of lhs and rhs
  //double > float, float > integers, signed > unsigned, wider integer > narrower integer
  if(lhs->isChar() && rhs->isChar())
    return lhs;
  else if(lhs->isChar())
    return rhs;
  else if(rhs->isChar())
    return lhs;
  else if(lhs->isInteger() && rhs->isInteger())
  {
    //two non-char integer types
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

Expression* StructType::findMember(Expression* thisExpr, string* names, size_t numNames, size_t& namesUsed)
{
  //Names inside modules of composing structs aren't visible
  //So, if a module name comes back, search for the name in this struct only.
  Scope* searchScope = scope;
  for(namesUsed = 0; namesUsed < numNames; namesUsed++)
  {
    string& n = names[namesUsed];
    namesUsed++;
    Name found = searchScope->lookup(n);
    if(found.item)
    {
      switch(found.kind)
      {
        case Name::SUBROUTINE:
        {
          //error if a static callable
          auto sd = (SubroutineDecl*) found.item;
          if(!sd->owner)
          {
            errMsgLoc(thisExpr, "Can't call static " <<
                (sd->isPure ? "func" : "proc") << " with a 'this' object");
          }
          INTERNAL_ASSERT(this == sd->owner);
          //OK, have a member callable
          auto s = new SubrOverloadExpr(thisExpr, sd);
          s->resolve();
          s->setLocation(thisExpr);
          return s;
        }
        case Name::VARIABLE:
        {
          Variable* v = (Variable*) found.item;
          StructMem* sm = new StructMem(thisExpr, v);
          sm->setLocation(thisExpr);
          sm->resolve();
          return sm;
        }
        case Name::MODULE:
        {
          Module* m = (Module*) found.item;
          searchScope = m->scope;
          break;
        }
        default:
          errMsgLoc(thisExpr, n <<
              " is not a member (variable/subroutine) of struct " << name);
      }
    }
    else if(namesUsed == 1)
    {
      //Only chance to try composition
      namesUsed--;
      for(size_t i = 0; i < members.size(); i++)
      {
        if(composed[i])
        {
          //Recursively try lookup in the composing struct
          StructMem* memberThis = new StructMem(thisExpr, members[i]);
          memberThis->setLocation(thisExpr);
          memberThis->resolve();
          StructType* composingStruct = (StructType*) memberThis->type;
          //But, members in submodules can't be accessed through composition
          //So only provide the first name.
          Expression* foundMem = composingStruct->findMember(memberThis, names, 1, namesUsed);
          if(foundMem)
            return foundMem;
          //otherwise, don't leak
          delete memberThis;
        }
      }
    }
  }
  //Didn't find anything
  return nullptr;
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
  if(auto otherUnion = dynamic_cast<UnionType*>(other))
  {
    //if every option of other can convert to this, good
    for(auto otherOp : otherUnion->options)
    {
      if(!canConvert(otherOp))
        return false;
    }
    return true;
  }
  else
  {
    //otherwise, check that other can convert to at least one option
    for(auto op : options)
    {
      if(op->canConvert(other))
        return true;
    }
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
    defaultVal = new UnionConstant(options[defaultType]->getDefaultValue(), this);
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

EnumType::EnumType(string n, Scope* enclosingScope)
{
  name = n;
  //"scope" encloses the enum constants
  scope = new Scope(enclosingScope, this);
}

void EnumType::resolveImpl()
{
  if(values.size() == 0)
  {
    errMsgLoc(this, "empty enum is not allowed. Try using \"type " << name << ";\" instead.");
  }
  //First, decide if a signed type must be used (because at least one
  //value is negative)
  bool useSigned = false;
  for(auto v : values)
  {
    if(v->isSigned)
    {
      useSigned = true;
      break;
    }
  }
  if(useSigned)
  {
    //int64_t is the biggest value that can be used, so check that all
    //positive values are <= int64_t's max value.
    uint64_t maxAllowed = (uint64_t) std::numeric_limits<int64_t>::max();
    for(auto v : values)
    {
      if(!v->isSigned && v->value > maxAllowed)
        errMsgLoc(v, "enum is signed, but this value doesn't fit in long");
    }
  }
  //finally, find the smallest type that fits all values
  int width = 8;
  //unsigned range: [0, 1 << width)
  //signed range:   [-(1 << (width - 1)), 1 << (width - 1))
  for(auto v : values)
  {
    if(useSigned)
    {
      int64_t sval = (int64_t) v->value;
      if(sval < -((int64_t) 1 << (width - 1)))
        width *= 2;
      else if(sval >= (int64_t) 1 << (width - 1))
        width *= 2;
    }
    else
    {
      if(v->value >= (uint64_t) 1 << width)
        width *= 2;
    }
    if(width == 64)
      break;
  }
  underlying = getIntegerType(width / 8, useSigned);
  resolved = true;
  defVal = new EnumExpr(values.front());
  defVal->resolve();
}

void EnumType::addAutomaticValue(string n, Node* location)
{
  //the next value is always 1 + the previous value, or 0 if
  //this is the first.
  uint64_t nextVal = 0;
  bool nextSigned = false;
  if(values.size())
  {
    //even if value really represents a 2s complement, adding
    //one still gives the next value.
    nextVal = values.back()->value + 1;
    nextSigned = values.back()->isSigned;
    if(nextVal == 0)
      nextSigned = false;
  }
  EnumConstant* newValue = new EnumConstant(n, nextVal, nextSigned);
  newValue->setLocation(location);
  newValue->et = this;
  scope->addName(newValue);
  values.push_back(newValue);
}

void EnumType::addPositiveValue(string n, uint64_t uval, Node* location)
{
  EnumConstant* newValue = new EnumConstant(n, uval);
  newValue->setLocation(location);
  newValue->et = this;
  scope->addName(newValue);
  values.push_back(newValue);
}

void EnumType::addNegativeValue(string n, int64_t sval, Node* location)
{
  uint64_t uval = static_cast<uint64_t>(sval);
  EnumConstant* newValue = new EnumConstant(n, uval, true);
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

Expression* EnumType::getDefaultValue()
{
  return defVal;
}

EnumExpr* EnumType::valueFromName(string n)
{
  EnumConstant* ec = (EnumConstant*) scope->lookup(n).item;
  if(ec)
    return new EnumExpr(ec);
  return nullptr;
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
  return other->isNumber();
}

Expression* CharType::getDefaultValue()
{
  return new IntConstant((uint64_t) 0, getCharType());
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

ElemExprType::ElemExprType(Expression* a) : arr(a)
{
  //0 means remove all dimensions
  reduction = 0;
}

ElemExprType::ElemExprType(Expression* a, int r)
  : arr(a), reduction(r) {}

void ElemExprType::resolveImpl()
{
  //should never get here,
  //ElemExprType must be replaced by another type in resolveType()
  INTERNAL_ERROR;
}

InferredReturnType::InferredReturnType(Block* b)
  : block(b) {}

void InferredReturnType::resolveImpl()
{
  INTERNAL_ERROR;
}

void resolveType(Type*& t)
{
  INTERNAL_ASSERT(t);
  Node* loc = t;
  if(t->resolved)
  {
    //nothing to do
    t = canonicalize(t);
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
        {
          AliasType* at = (AliasType*) found.item;
          at->resolve();
          t = at->actual;
          break;
        }
        default:
          errMsgLoc(unres, "name " << mem << " does not refer to a type");
      }
      t->resolve();
    }
    else if(unres->t.is<UnresolvedType::Tuple>())
    {
      auto& tupleList = unres->t.get<UnresolvedType::Tuple>();
      t = getTupleType(tupleList.members);
    }
    else if(unres->t.is<UnresolvedType::Union>())
    {
      //first, resolve the elements of the union
      auto& unionList = unres->t.get<UnresolvedType::Union>();
      t = getUnionType(unionList.members);
    }
    else if(unres->t.is<UnresolvedType::Map>())
    {
      auto& kv = unres->t.get<UnresolvedType::Map>();
      t = getMapType(kv.key, kv.value);
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
      t = getArrayType(t, unres->arrayDims);
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
    if(eet->reduction == 0 || arrType->dims == eet->reduction)
    {
      //remove all dimensions, giving a singular type
      t = arrType->elem;
    }
    else
    {
      //only remove "reduction" dimensions, producing a smaller array type
      t = getArrayType(arrType->elem, arrType->dims - eet->reduction);
    }
  }
  else if(InferredReturnType* irt = dynamic_cast<InferredReturnType*>(t))
  {
    Block* topBlock = irt->block;
    topBlock->resolve();
    //find all return statements in the block, resolve the returned values,
    //and make sure all types match.
    stack<Block*> toVisit;
    toVisit.push(topBlock);
    t = nullptr;
    while(toVisit.size())
    {
      Block* block = toVisit.top();
      toVisit.pop();
      for(auto s : block->stmts)
      {
        if(Block* subblock = dynamic_cast<Block*>(s))
          toVisit.push(subblock);
        else if(Return* ret = dynamic_cast<Return*>(s))
        {
          if(!ret->value)
            errMsgLoc(ret, "Trying to infer function return type, but this return has no value");
          if(t && !typesSame(t, ret->value->type))
            errMsgLoc(ret, "Trying to infer function return type, but this return type differs from a previous one");
          t = ret->value->type;
        }
      }
    }
    if(!t)
    {
      errMsgLoc(topBlock, "Trying to infer function return type, but body contained no return statements");
    }
  }
  else
  {
    t->resolve();
  }
  t = canonicalize(t);
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

SimpleType* getVoidType()
{
  return (SimpleType*) primitives[Prim::VOID];
}

SimpleType* getErrorType()
{
  return (SimpleType*) primitives[Prim::ERROR];
}

BoolType* getBoolType()
{
  return (BoolType*) primitives[Prim::BOOL];
}

IntegerType* getCharType()
{
  return (IntegerType*) primitives[Prim::CHAR];
}

ArrayType* getStringType()
{
  return (ArrayType*) getArrayType(primitives[Prim::CHAR], 1);
}

IntegerType* getLongType()
{
  return (IntegerType*) primitives[Prim::LONG];
}

IntegerType* getULongType()
{
  return (IntegerType*) primitives[Prim::ULONG];
}

