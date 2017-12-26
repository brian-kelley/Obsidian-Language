#include "TypeSystem.hpp"
#include "Variable.hpp"
#include "Expression.hpp"
#include "Subroutine.hpp"

using namespace Parser;

/***********************/
/* Type and subclasses */
/***********************/

struct ModuleScope;

extern ModuleScope* global;

namespace TypeSystem
{
vector<Type*> primitives;
map<string, Type*> primNames;
vector<StructType*> structs;
vector<BoundedType*> boundedTypes;
set<ArrayType*, ArrayCompare> arrays;
set<TupleType*, TupleCompare> tuples;
set<UnionType*, UnionCompare> unions;
set<MapType*, MapCompare> maps;
set<CallableType*, CallableCompare> callables;

//these are created in MiddleEnd
DeferredTypeLookup* typeLookup;
DeferredTraitLookup* traitLookup;

bool Type::canConvert(Expression* other)
{
  //Basic behavior here: if other has a known type, check if that can convert
  if(other->type)
    return canConvert(other->type);
  return false;
}

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
    //now look up the type for the singular element type
    type->arrayDims = 0;
    Type* elemType = lookupType(type, scope);
    //restore original type to preserve parse tree (just in case)
    type->arrayDims = dims;
    return getArrayType(elemType, dims);
  }
  else if(type->t.is<TypeNT::Prim>())
  {
    return primitives[(int) type->t.get<TypeNT::Prim>()];
  }
  else if(type->t.is<Member*>())
  {
    //intercept special case: "T" inside a trait decl
    auto mem = type->t.get<Member*>();
    if(mem->names.size() == 1 && mem->names[0] == "T")
    {
      if(auto ts = dynamic_cast<TraitScope*>(scope))
      {
        return ts->ttype;
      }
    }
    Name n = scope->findName(mem);
    if(!n.item)
      return nullptr;
    if(n.kind == Name::STRUCT ||
        n.kind == Name::ENUM ||
        n.kind == Name::BOUNDED_TYPE)
    {
      return (Type*) n.item;
    }
    else if(n.kind == Name::TYPEDEF)
    {
      return ((AliasType*) n.item)->actual;
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
    }
    return getTupleType(members);
  }
  else if(type->t.is<UnionTypeNT*>())
  {
    auto utNT = type->t.get<UnionTypeNT*>();
    vector<Type*> options;
    for(auto t : utNT->types)
    {
      options.push_back(lookupType(t, scope));
    }
    return getUnionType(options);
  }
  else if(type->t.is<MapTypeNT*>())
  {
    auto mtNT = type->t.get<MapTypeNT*>();
    return getMapType(lookupType(mtNT->keyType, scope), lookupType(mtNT->valueType, scope));
  }
  else if(type->t.is<SubroutineTypeNT*>())
  {
    auto stNT = type->t.get<SubroutineTypeNT*>();
    //find the owner struct (from scope), if it exists
    StructType* owner = nullptr;
    bool pure = stNT->isPure;
    bool nonterm = stNT->nonterm;
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
    vector<Type*> argTypes;
    for(auto param : stNT->params)
    {
      if(param->type.is<TypeNT*>())
      {
        argTypes.push_back(lookupType(param->type.get<TypeNT*>(), scope));
      }
      else
      {
        auto bt = param->type.get<BoundedTypeNT*>();
        //attempt to look up the bounded type by name
        Member m;
        m.names.push_back(bt->localName);
        TypeNT wrapper;
        wrapper.t = &m;
        Type* namedBT = lookupType(&wrapper, scope);
        argTypes.push_back(namedBT);
      }
    }
    return getSubroutineType(owner, pure, nonterm, retType, argTypes);
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

Trait* lookupTrait(Parser::Member* name, Scope* scope)
{
  Name n = scope->findName(name);
  if(!n.item)
    return nullptr;
  if(n.kind != Name::TRAIT)
  {
    ERR_MSG(*name << " is not a trait");
  }
  return (Trait*) n.item;
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

BoundedType::BoundedType(Parser::BoundedTypeNT* tt, Scope* s)
{
  boundedTypes.push_back(this);
  name = tt->localName;
  traits.resize(tt->traits.size());
  for(size_t i = 0; i < tt->traits.size(); i++)
  {
    TraitLookup tl(tt->traits[i], s);
    traitLookup->lookup(tl, traits[i]);
  }
}

bool BoundedType::canConvert(Type* other)
{
  //only requirement is that other implements all traits of this
  for(auto t : traits)
  {
    if(!other->implementsTrait(t))
      return false;
  }
  return true;
}

void BoundedType::check()
{
  //all the traits must be loaded, so make a list of the subroutines
  //that any implmentation of this must have
  for(auto t : traits)
  {
    for(size_t i = 0; i < t->callables.size(); i++)
    {
      string& sname = t->subrNames[i];
      if(subrs.find(sname) != subrs.end())
      {
        ERR_MSG("bounded type " << name << " is invalid because subroutines with name "
            << sname << " exist in more than one of its traits");
        subrs[sname] = t->callables[i];
      }
    }
  }
}

bool BoundedType::canConvert(Expression* other)
{
  return canConvert(other->type);
}

/***********/
/*  Trait  */
/***********/

Trait::Trait(Parser::TraitDecl* td, TraitScope* s)
{
  name = td->name;
  scope = s;
  //pre-allocate subr and names vectors
  subrNames.resize(td->members.size());
  callables.resize(td->members.size());
  //use deferred callable type lookup for members
  for(size_t i = 0; i < td->members.size(); i++)
  {
    Parser::SubroutineNT* subr = td->members[i];
    if(subr->isStatic)
    {
      ERR_MSG("subroutine " << subr->name << " in trait " << name << " is static");
    }
    if(subr->body)
    {
      ERR_MSG("subroutine " << subr->name << " in trait " << name << " has a body which is not allowed");
    }
    subrNames[i] = subr->name;
    //Make a new SubroutineTypeNT from the informtion in subr
    auto subrType = new Parser::SubroutineTypeNT;
    subrType->retType = subr->retType;
    subrType->params = subr->params;
    subrType->isPure = subr->isPure;
    subrType->isStatic = subr->isStatic;
    subrType->nonterm = subr->nonterm;
    TypeLookup lookupArgs(subrType, s);
    typeLookup->lookup(lookupArgs, (Type*&) callables[i]);
  }
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
  //Load traits using deferred trait lookup
  traits.resize(sd->traits.size());
  for(size_t i = 0; i < sd->traits.size(); i++)
  {
    TraitLookup lookupArgs(sd->traits[i], enclosingScope);
    traitLookup->lookup(lookupArgs, traits[i]);
  }
  checked = false;
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
      if(!(members[i]->type->canConvert(cl->members[i])))
      {
        canConvert = false;
        break;
      }
    }
    return canConvert;
  }
  return false;
}

//called once per struct after the scope/type/variable pass of middle end
void StructType::check()
{
  if(checked)
    return;
  //check for membership cycles
  checking = true;
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
  //"this" members have top priority, then composed members in order of declaration
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
        //for each member subr of memberStruct, if its signature matches an existing subroutine,
        //replace existing
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
  //go through each trait, and make sure this supports an exactly matching
  //subroutine (name, purity, nontermness, staticness, retun type, arg types)
  //(ownerStruct does NOT have to match because of composition)
  for(auto t : traits)
  {
    for(size_t i = 0; i < t->subrNames.size(); i++)
    {
      //get the name directly inside struct scope (must exist)
      string subrName = t->subrNames[i];
      if(interface.find(subrName) == interface.end())
      {
        ERR_MSG("struct " << name << " doesn't implement subroutine " <<
            subrName << " required by trait " << t->name);
      }
      CallableType* subrType = t->callables[i];
      if(!subrType->sameExceptOwner(interface[subrName].subr->type))
      {
        ERR_MSG("subroutine " << name << "." << subrName <<
            "has a different signature than in trait " << t->name); 
      }
    }
  }
  checked = true;
  checking = false;
}

bool StructType::contains(Type* t)
{
  if(t == this)
    return true;
  for(auto mem : members)
  {
    if(mem->type == t || mem->type->contains(t))
    {
      return true;
    }
  }
  return false;
}

bool StructType::implementsTrait(Trait* t)
{
  assert(checked);
  return find(traits.begin(), traits.end(), t) != traits.end();
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
  MapType* mt = dynamic_cast<MapType*>(other);
  if(!mt)
    return false;
  return mt->key == key && mt->value == value;
}

bool MapType::canConvert(Expression* other)
{
  return canConvert(other->type);
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

bool AliasType::canConvert(Expression* other)
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

bool CallableType::canConvert(Expression* other)
{
  return other->type && canConvert(other->type);
}

bool CallableType::sameExceptOwner(CallableType* other)
{
  return pure == other->pure &&
    nonterminating == other->nonterminating &&
    returnType == other->returnType &&
    argTypes == other->argTypes;
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

TType::TType(TraitScope* ts)
{
  scope = ts;
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
  return canConvert(other->type);
}

} //namespace TypeSystem

