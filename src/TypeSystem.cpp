#include "TypeSystem.hpp"

using namespace std;
using namespace Parser;

/***********************/
/* Type and subclasses */
/***********************/

vector<TupleType*> Type::tuples;
map<string, Type*> Type::primitives;

Type::Type(Scope* enclosingScope)
{
  enclosing = enclosingScope;
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
  table.emplace_back(new AliasType("f64", &table.back()));
  ADD_PRIM;
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

Type* Type::getArrayType(Parser::TypeNT* type, Scope* usedScope, int arrayDims)
{
  Type* underlying = getTypeOrUndef(type, usedScope);
}

Type* Type::getTupleType(Parser::TupleType* tt, Scope* usedScope)
{
}

Type* Type::getTypeOrUndef(Parser::TypeNT* type, Scope* usedScope, Type* usage)
{
  Type* gotten = getType(type, usedScope);
  if(!gotten)
  {
  UndefType(string name, Scope* enclosing, Type* usage);
    UndefType ut = AP(new UndefType(name));
  }
  else
  {
    return gotten;
  }
}

StructType(string name, Scope* enclosingScope)
{
  this->name = name;
  this->enclosing = enclosingScope;
}

string StructType::getCName()
{
}

bool StructType::hasFunc(ProcType& type)
{
}

bool StructType::hasProc(ProcType& type)
{
}

TupleType::TupleType(TupleType& tt)
{
}

string TupleType::getCName()
{
}

AliasType::AliasType(string newName, Type* t)
{
}

AliasType::AliasType(Typedef& td)
{
}

string AliasType::getCName()
{
  return actual->getCName();
}

EnumType::EnumType(Parser::Enum& e)
{
}

string EnumType::getCName()
{
}

IntegerType::IntegerType(string name, int size, bool sign)
{
  this->name = name;
  this->size = size;
  this->isSigned = sign;
}

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

FloatType::FloatType(string name, int size)
{
  this->name = name;
  this->size = size;
}

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

string StringType::getCName()
{
  return "string";
}

string BoolType::getCName()
{
  return "bool";
}

UndefType(string name, Scope* enclosing, Type* usage)
{
}

UndefType(Parser::TypeNT* t, Scope* enclosing, Type* usage)
{
}

