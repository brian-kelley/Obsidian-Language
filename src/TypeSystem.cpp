#include "TypeSystem.hpp"

using namespace std;
using namespace Parser;

/***********************/
/* Type and subclasses */
/***********************/

extern AP(ModuleScope) global;

vector<TupleType*> Type::tuples;
map<string, Type*> Type::primitives;
vector<UndefType*> UndefType::instances;

Type::Type(Scope* enclosingScope)
{
  enclosing = enclosingScope;
  enclosingScope->types.push_back(this);
}

void Type::createBuiltinTypes(Scope* global)
{
#define ADD_PRIM primitives.push_back(&table.back().get())
  vector<AP(Type)>& table = global->types;
  table.emplace_back(new BoolType);
  ADD_PRIM;
  table.emplace_back(new IntegerType("char", 1, true));
  ADD_PRIM;
  table.emplace_back(new AliasType("i8", &table.back()));
  table.emplace_back(new IntegerType("uchar", 1, false));
  ADD_PRIM;
  table.emplace_back(new AliasType("u8", &table.back()));
  table.emplace_back(new IntegerType("short", 2, true));
  ADD_PRIM;
  table.emplace_back(new AliasType("i16", &table.back()));
  table.emplace_back(new IntegerType("ushort", 2, false));
  ADD_PRIM;
  table.emplace_back(new AliasType("u16", &table.back()));
  table.emplace_back(new IntegerType("int", 4, true));
  ADD_PRIM;
  table.emplace_back(new AliasType("i32", &table.back()));
  table.emplace_back(new IntegerType("uint", 4, false));
  ADD_PRIM;
  table.emplace_back(new AliasType("u32", &table.back()));
  table.emplace_back(new IntegerType("long", 8, true));
  ADD_PRIM;
  table.emplace_back(new AliasType("i64", &table.back()));
  table.emplace_back(new IntegerType("ulong", 8, false));
  ADD_PRIM;
  table.emplace_back(new AliasType("u64", &table.back()));
  table.emplace_back(new FloatType("float", 4));
  ADD_PRIM;
  table.emplace_back(new AliasType("f32", &table.back()));
  table.emplace_back(new FloatType("double", 8));
  ADD_PRIM;
  table.emplace_back(new AliasType("f64", &table.back()));
  table.emplace_back(new StringType);
  ADD_PRIM;
#undef ADD_PRIM
}

//Get the type table entry, given the local usage name and current scope
//If type not defined, return NULL
Type* Type::getType(string localName, Scope* usedScope)
{
  Parser::Member mem;
  mem.owner = localName;
  mem.member = AP(Member)(nullptr);
  return getType(&mem, usedScope, true);
}

Type* Type::getType(Parser::Member* localName, Scope* usedScope, bool searchUp)
{
  if(localName->member.get())
  {
    //get scope with name localName->owner, then try there with tail of localName
    for(auto& it : usedScope->children)
    {
      if(it->getLocalName() == localName->owner)
      {
        //can only be one child scope with that name
        //only need to search this one scope (it)
        return getType(localName->member, it, false);
      }
    }
  }
  else
  {
    //localName is just the type name, search for it in this scope
    for(auto& it : usedScope->types)
    {
      if(it->getLocalName() == localName->owner)
      {
        return it;
      }
    }
  }
  if(searchUp && usedScope->parent)
  {
    return getType(localName, usedScope->parent, false);
  }
  else
  {
    return NULL;
  }
}

StructType(string name, Scope* enclosingScope)
{
  this->name = name;
  this->enclosing = enclosingScope;
}

bool StructType::hasFunc(FuncType* type)
{
}

bool StructType::hasProc(ProcType* type)
{
}

TupleType::TupleType(TupleTypeNT* tt, Scope* currentScope) : Type(currentScope)
{
  for(size_t i = 0; i < tt->members.size(); i++)
  {
    TypeNT* typeNT = tt->members[i];
    Type* type = getTypeOrUndef(typeNT, currentScope, this, i);
    members.push_back(type);
  }
}

Type* Type::getTypeOrUndef(TypeNT* nt, Scope* currentScope, Type* usage, int tupleIndex)
{
  Type* lookup = getType(td->type, currentScope);
  if(!lookup)
  {
    //UndefType(string name, Scope* enclosing, Type* usageType) : Type(enclosing)
    //Failed lookup: underlying parsed type must be a Member
    //Must use member's full name for unambiguous aliased type
    return new UndefType(nt->t.get<AP(Member)>().get(), currentScope, usage, tupleIndex);
  }
  else
  {
    //lookup successful so use known underlying type
    return lookup;
  }
}

AliasType::AliasType(Typedef* td, Scope* current) : Type(current)
{
  name = td->ident;
  actual = getTypeOrUndef(td->type, current, this);
}

EnumType::EnumType(Parser::Enum* e, Scope* current) : Type(current)
{
  name = e->name;
  for(auto& it : e->items)
  {
    values[it->name] = it->value->val;
  }
}

IntegerType::IntegerType(string name, int size, bool sign)
{
  this->name = name;
  this->size = size;
  this->isSigned = sign;
}

/*
string IntegerType::getCName()
{
  if(size == 1 && isSigned)
    return "char";
  else if(size == 1)
    return "uchar";
  else if(size == 2 && isSigned)
    return "short";
  else if(size == 2)
    return "ushort";
  else if(size == 4 && isSigned)
    return "int";
  else if(size == 4)
    return "uint";
  else if(size == 8 && isSigned)
    return "long";
  else if(size == 8)
    return "ulong";
  else
  {
    //todo: support arbitrary-sized ints as builtin types
  }
}
*/
FloatType::FloatType(string name, int size)
{
  this->name = name;
  this->size = size;
}

/*
string FloatType::getCName()
{
  if(size == 4)
  {
    return "float";
  }
  else if(size == 8)
  {
    return "double"
  }
}
*/

UndefType::UndefType(Member* mem, Scope* enclosing, Type* usageType, int tupleIndex) : Type(enclosing)
{
  for(Member* iter = mem; iter; iter = iter->mem.get())
  {
    name.push_back(iter->owner);
  }
  //use short-circuit eval to set the usage variant correctly
  (usage = dynamic_cast<StructType>()) ||
    (usage = dynamic_cast<UnionType>()) ||
    (usage = dynamic_cast<ArrayType>()) ||
    (usage = dynamic_cast<TupleType>()) ||
    (usage = dynamic_cast<AliasType>());
  this->tupleIndex = tupleIndex;
  instances.push_back(this);
}

UndefType::UndefType(string name, Scope* enclosing, Type* usageType) : Type(enclosing)
{
  this->name.push_back(iter->owner);
  //use short-circuit eval to set the usage variant correctly
  (usage = dynamic_cast<StructType>()) ||
    (usage = dynamic_cast<UnionType>()) ||
    (usage = dynamic_cast<ArrayType>()) ||
    (usage = dynamic_cast<TupleType>()) ||
    (usage = dynamic_cast<AliasType>());
  instances.push_back(this);
}

