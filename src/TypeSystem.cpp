#include "TypeSystem.hpp"
#include "Variable.hpp"
#include "Expression.hpp"
#include "Subroutine.hpp"

using namespace Parser;

/***********************/
/* Type and subclasses */
/***********************/

struct ModuleScope;

extern Scope* global;

vector<Type*> primitives;

namespace TypeSystem
{
map<string, Type*> primNames;
vector<StructType*> structs;
set<ArrayType*, ArrayCompare> arrays;
set<TupleType*, TupleCompare> tuples;
set<UnionType*, UnionCompare> unions;
set<MapType*, MapCompare> maps;
set<CallableType*, CallableCompare> callables;
set<EnumType*> enums;

//these are created in MiddleEnd
DeferredTypeLookup* typeLookup;

void createBuiltinTypes()
{
  using Parser::TypeNT;
  //primitives has same size as the enum Parser::TypeNT::Prim
  primitives.resize(14);
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
  primitives[TypeNT::ERROR] = new ErrorType;
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
  primNames["Error"] = primitives[TypeNT::ERROR];
  //string is a builtin alias for char[] (not a primitive)
  global->addName(new AliasType(
        "string", getArrayType(primitives[TypeNT::CHAR], 1)));
  global->addName(new AliasType("i8", primitives[TypeNT::BYTE]));
  global->addName(new AliasType("u8", primitives[TypeNT::UBYTE]));
  global->addName(new AliasType("i16", primitives[TypeNT::SHORT]));
  global->addName(new AliasType("u16", primitives[TypeNT::USHORT]));
  global->addName(new AliasType("i32", primitives[TypeNT::INT]));
  global->addName(new AliasType("u32", primitives[TypeNT::UINT]));
  global->addName(new AliasType("i64", primitives[TypeNT::LONG]));
  global->addName(new AliasType("u64", primitives[TypeNT::ULONG]));
  global->addName(new AliasType("f32", primitives[TypeNT::FLOAT]));
  global->addName(new AliasType("f64", primitives[TypeNT::DOUBLE]));
}

string typeErrorMessage(TypeLookup& lookup)
{
  Oss oss;
  if(lookup.type.is<Parser::TypeNT*>())
  {
    oss << "unknown type at " <<
      *lookup.type.get<Parser::TypeNT*>();
  }
  else if(lookup.type.is<Parser::SubroutineTypeNT*>())
  {
    oss << "unknown type at " <<
      *lookup.type.get<Parser::SubroutineTypeNT*>();
  }
  else
  {
    INTERNAL_ERROR;
  }
  return oss.str();
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

Type* getSubroutineType(StructType* owner, bool pure, bool nonterm, Type* retType, vector<Type*>& argTypes)
{
  if(retType == nullptr)
    return nullptr;
  for(auto arg : argTypes)
  {
    if(arg == nullptr)
      return nullptr;
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
  options.push_back(primitives[TypeNT::ERROR]);
  return getUnionType(options);
}

Type* lookupTypeDeferred(TypeLookup& args)
{
  if(args.type.is<Parser::TypeNT*>())
  {
    return lookupType(args.type.get<Parser::TypeNT*>(), args.scope);
  }
  else
  {
    return lookupSubroutineType(args.type.get<Parser::SubroutineTypeNT*>(), args.scope);
  }
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

/***************/
/* Struct Type */
/***************/

StructType::StructType(Parser::StructDecl* sd, Scope* enclosingScope, StructScope* sscope)
{
  structs.push_back(this);
  this->name = sd->name;
  this->structScope = sscope;
  //have struct scope point back to this
  sscope->type = this;
  checked = false;
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

//called once per struct after the scope/type/variable pass of middle end
void StructType::check()
{
  if(checked)
    return;
  //check for membership cycles
  if(contains(this))
  {
    ERR_MSG("struct " << name << " has itself as a member");
  }
  //then make sure that all members of struct type have been checked
  //(having checked members is necessary to analyze traits and composition)
  for(auto mem : members)
  {
    if(auto st = dynamic_cast<StructType*>(mem->type))
    {
      if(!st->checked)
        st->check();
    }
  }
  //now build the "interface": all direct subroutine members of this and composed members
  //overriding priority: direct members > 1st composed > 2nd composed > ...
  //for subroutines which are available through composition, need to know which member it belongs to
  for(auto& decl : structScope->names)
  {
    Name n = decl.second;
    if(n.kind == Name::SUBROUTINE)
    {
      //have a subroutine, add to interface
      interface[decl.first] = IfaceMember(nullptr, (Subroutine*) n.item);
    }
  }
  for(size_t i = 0; i < members.size(); i++)
  {
    if(composed[i])
    {
      StructType* memberStruct = dynamic_cast<StructType*>(members[i]->type);
      if(memberStruct)
      {
        //for each member subr of memberStruct, if its name isn't already in interface, add it
        StructScope* memScope = memberStruct->structScope;
        for(auto& decl : memScope->names)
        {
          Name n = decl.second;
          if(n.kind == Name::SUBROUTINE && interface.find(decl.first) == interface.end())
          {
            interface[decl.first] = IfaceMember(members[i], (Subroutine*) n.item);
          }
        }
      }
    }
  }
  checked = true;
}

bool StructType::contains(Type* t)
{
  for(auto mem : members)
  {
    if(mem->type == t || mem->type->contains(t))
    {
      return true;
    }
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
  assert(elemType);
  assert(ndims > 0);
  this->dims = ndims;
  this->elem = elemType;
  //If 1-dimensional, subtype is just elem
  //Otherwise is array with one fewer dimension
  subtype = (ndims == 1) ? elem : getArrayType(elemType, dims - 1);
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

bool ArrayType::contains(Type* t)
{
  if(t == elem)
    return true;
  ArrayType* at = dynamic_cast<ArrayType*>(t);
  if(at && at->elem == elem && dims > at->dims)
    return true;
  return false;
}

void ArrayType::check()
{
  if(contains(this))
  {
    ERR_MSG("array type (" << dims << " dims of " << elem->getName() << ") contains itself");
  }
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
  this->members = mems;
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

bool TupleType::contains(Type* t)
{
  for(auto mem : members)
  {
    if(mem->contains(t))
      return true;
  }
  return false;
}

void TupleType::check()
{
  if(contains(this))
  {
    ERR_MSG("tuple contains itself (directly or indirectly)");
  }
}

bool TupleCompare::operator()(const TupleType* lhs, const TupleType* rhs)
{
  return lexicographical_compare(lhs->members.begin(), lhs->members.end(),
      rhs->members.begin(), rhs->members.end());
}

/************/
/* Map Type */
/************/

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

bool MapType::contains(Type* t)
{
  return key->contains(t) || value->contains(t);
}

void MapType::check()
{
  if(contains(this))
  {
    ERR_MSG("map's key or value type contains the map itself");
  }
}

bool MapCompare::operator()(const MapType* lhs, const MapType* rhs)
{
  return (lhs->key < rhs->key) || (lhs->key == rhs->key && lhs->value < rhs->value);
}

/**************/
/* Alias Type */
/**************/

AliasType::AliasType(Typedef* td, Scope* scope)
{
  name = td->ident;
  decl = td;
  TypeLookup args = TypeLookup(td->type, scope);
  typeLookup->lookup(args, actual);
}

AliasType::AliasType(string alias, Type* underlying)
{
  name = alias;
  actual = underlying;
  decl = NULL;
}

bool AliasType::canConvert(Type* other)
{
  return actual->canConvert(other);
}

bool AliasType::contains(Type* t)
{
  return actual->contains(t);
}

/*************/
/* Enum Type */
/*************/

EnumType::EnumType(Parser::Enum* e, Scope* current)
{
  enums.insert(this);
  name = e->name;
  set<int64_t> used;
  int64_t autoVal = 0;
  for(auto item : e->items)
  {
    EnumConstant* ec = new EnumConstant;
    ec->et = this;
    ec->name = item->name;
    if(item->value)
    {
      ec->value = item->value->val;
    }
    else
    {
      ec->value = autoVal;
    }
    autoVal = ec->value + 1;
    if(used.find(ec->value) != used.end())
    {
      ERR_MSG("in enum " << name << ", key " <<
          ec->name << " has repeated value");
    }
    used.insert(ec->value);
    current->addName(ec);
    values.push_back(ec);
  }
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
  this->name = typeName;
  this->size = sz;
  this->isSigned = sign;
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
  this->name = typeName;
  this->size = sz;
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

CallableType::CallableType(bool isPure, Type* retType, vector<Type*>& args, bool nonterm)
{
  pure = isPure;
  returnType = retType;
  argTypes = args;
  nonterminating = nonterm;
  ownerStruct = NULL;
}

CallableType::CallableType(bool isPure, StructType* owner, Type* retType, vector<Type*>& args, bool nonterm)
{
  pure = isPure;
  returnType = retType;
  argTypes = args;
  nonterminating = nonterm;
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
  if(ownerStruct != ct->ownerStruct)
  {
    return false;
  }
  if(!nonterminating && ct->nonterminating)
    return false;
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
  if(!lhs->nonterminating && rhs->nonterminating)
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

void resolveType(Type*& t, bool err)
{
  if(t->isResolved())
  {
    //nothing to do
    return;
  }
  UnresolvedType* unres = dynamic_cast<UnresolvedType*>(t);
  if(!unres)
    return;
  Type* finalType = nullptr;
  switch(unres->k)
  {
    case UnresolvedType::Primitive:
      finalType = primitives[unres->data.primitive];
      break;
    case UnresolvedType::NamedType:
      {
        //search for member from scope
        Name found = unres->scope->findName(unres->data.m);
        if(!found.item && err)
        {
          ERR_MSG("unknown type " << *unres->data.m);
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
            ERR_MSG("name " << *unres->data.m << " does not refer to a type");
        }
        break;
      }
    case UnresolvedType::Tuple:
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
        break;
      }
    case UnresolvedType::Union:
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
        break;
      }
    case UnresolvedType::Map:
      {
        Type*& key = unres->data.mt.key;
        Type*& value = unres->data.mt.value;
        resolveType(key, err);
        resolveType(value, err);
        if(key->isResolved() && value->isResolved())
        {
          finalType = getMapType(key, value);
        }
        break;
      }
    case UnresolvedType::SubrType:
      {
        //walk up scope tree to see if in a non-static context
        Struct* ownerStruct = unres->scope->getStructContext();
        auto& ct = unres->data.ct;
        bool allResolved = true;
        resolveType(ct.retType, err);
        if(!ct.retType->isResolved())
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
              ct.nonterm, ct.retType, ct.params);
        }
        break;
      }
  }
  if(!finalType)
  {
    //can't apply array dimensions, so return early
    if(err)
    {
      //shouldn't get here: any error should have been produced
      //above when attempting to resolve dependency types
      INTERNAL_ERROR;
    }
    return;
  }
  //if arrayDims is 0, this is a no-op
  finalType = getArrayType(finalType, ct.arrayDims);
  //finally, replace unres with finalType
  t = finalType;
}

} //namespace TypeSystem

