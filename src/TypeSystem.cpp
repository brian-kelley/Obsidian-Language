#include "TypeSystem.hpp"
#include "Variable.hpp"
#include "Expression.hpp"
#include "Subroutine.hpp"

/***********************/
/* Type and subclasses */
/***********************/

extern Scope* global;

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
  global->addName(new AliasType(
        "string", getArrayType(primitives[Prim::CHAR], 1)));
  global->addName(new AliasType("i8", primitives[Prim::BYTE]));
  global->addName(new AliasType("u8", primitives[Prim::UBYTE]));
  global->addName(new AliasType("i16", primitives[Prim::SHORT]));
  global->addName(new AliasType("u16", primitives[Prim::USHORT]));
  global->addName(new AliasType("i32", primitives[Prim::INT]));
  global->addName(new AliasType("u32", primitives[Prim::UINT]));
  global->addName(new AliasType("i64", primitives[Prim::LONG]));
  global->addName(new AliasType("u64", primitives[Prim::ULONG]));
  global->addName(new AliasType("f32", primitives[Prim::FLOAT]));
  global->addName(new AliasType("f64", primitives[Prim::DOUBLE]));
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

Type* getIntegerType(int bytes, bool isSigned)
{
  switch(bytes)
  {
    case 1:
      if(isSigned)  return primitives[Prim::BYTE];
      else          return primitives[Prim::UBYTE];
    case 2:
      if(isSigned)  return primitives[Prim::SHORT];
      else          return primitives[Prim::USHORT];
    case 4:
      if(isSigned)  return primitives[Prim::INT];
      else          return primitives[Prim::UINT];
    case 8:
      if(isSigned)  return primitives[Prim::LONG];
      else          return primitives[Prim::ULONG];
    default: INTERNAL_ERROR;
  }
  return NULL;
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
  //if all member resolutions succeed, this union is now
  //permanently resolved
}

bool UnionType::canConvert(Type* other)
{
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

AliasType::AliasType(string alias, Type* underlying)
{
  name = alias;
  actual = underlying;
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
  scope = new Scope(enclosingScope, this);
  resolved = true;
}

void EnumType::addValue(string valueName)
{
  addValue(valueName, values.back()->value + 1);
}

void EnumType::addValue(string valueName, int64_t value)
{
  if(valueSet.find(value) != valueSet.end())
  {
    errMsgLoc(this, "enum value " << value << " repeated");
  }
  valueSet.insert(value);
  EnumConstant* newValue = new EnumConstant;
  newValue->et = this;
  newValue->name = valueName;
  newValue->value = value;
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

CallableType::CallableType(bool isPure, Type* retType, vector<Type*>& args);
{
  pure = isPure;
  returnType = retType;
  argTypes = args;
  ownerStruct = NULL;
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

void ExprType::resolve(bool)
{
  //should never get here,
  //ExprType must be replaced by another type in resolveType()
  INTERNAL_ERROR;
}

ElemExprType::ElemExprType(Expression* a) : arr(a) {}

void ElemExprType::resolve(bool)
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
    if(unres->t.is<Prim>())
    {
      finalType = primitives[unres->t.get<Prim>()];
    }
    else if(unres->t.is<Member*>())
    {
      Name found = unres->scope->findName(unres->t.get<Member*>());
      //name wasn't found
      //if this is the last chance to resolve type, is an error
      if(!found.item && final)
      {
        errMsgLoc(unres, "unknown type " << *unres->data.m);
      }
      switch(found.kind)
      {
        case Name::STRUCT:
          finalType = (Struct*) found.item;
          break;
        case Name::ENUM:
          finalType = (Enum*) found.item;
          break;
        case Name::ALIAS:
          finalType = ((Alias*) found.item)->actual;
          break;
        default:
          errMsgLoc(unres, "name " << *unres->data.m << " does not refer to a type");
      }
    }
    else if(unres->t.is<UnresolvedType::TupleList>())
    {
      //resolve member types individually
      bool allResolved = true;
      for(Type*& mem : unres->data.t)
      {
        resolveType(mem, err);
        if(!mem->isResolved())
          allResolved = false;
      }
      if(allResolved)
      {
        finalType = getTupleType(unres->data.t);
      }
    }
    else if(unres->t.is<UnresolvedType::UnionList>())
    {
      //resolve member types individually
      bool allResolved = true;
      for(Type*& option : unres->data.u)
      {
        resolveType(option, err);
        if(!t->isResolved())
          allResolved = false;
      }
      if(allResolved)
      {
        finalType = getUnionType(unres->data.u);
      }
    }
    else if(unres->t.is<UnresolvedType::Map>())
    {
      Type*& key = unres->data.mt.key;
      Type*& value = unres->data.mt.value;
      resolveType(key, err);
      resolveType(value, err);
      if(key->isResolved() && value->isResolved())
      {
        finalType = getMapType(key, value);
      }
    }
    else if(unres->t.is<UnresolvedType::Callable>())
    {
      //walk up scope tree to see if in a non-static context
      Struct* ownerStruct = unres->scope->getStructContext();
      Callable& ct = unres->t.get<UnresolvedType::Callable>();
      bool allResolved = true;
      resolveType(ct.returnType, err);
      if(!ct.returnType->isResolved())
      {
        allResolved = false;
      }
      for(auto& param : ct.params)
      {
        resolveType(param, err);
        if(!param->isResolved())
        {
          allResolved = false;
        }
      }
      if(allResolved)
      {
        finalType = getSubroutineType(ownerStruct, ct.pure,
            ct.retType, ct.params);
      }
    }
    if(!finalType)
    {
      //can't apply array dimensions, so return early
      return;
    }
    //if arrayDims is 0, this is a no-op
    finalType = getArrayType(finalType, unres->arrayDims);
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

