#include "Type.hpp"

static vector<Type*> Type::table;

void Type::registerType(Type* newType)
{
  //check if type already exists
  for(Type* t : table)
  {
    if(newType->name == t->name && *newType != *t)
      errAndQuit(string("Type ") + t->name + " redeclared differently");
  }
  //safe to add type
  table.push_back(newType);
}

bool Type::typeExists(string name)
{
  for(Type* t : table)
  {
    if(t->name == name)
      return true;
  }
  return false;
}

Type* Type::typeFromName(string name)
{
  for(Type* t : table)
  {
    if(t->name == name)
      return t;
  }
  return nullptr;
}

bool Type::operator==(const Type& rhs)
{
  if(name == rhs.name &&
     isPOD() == rhs.isPOD() &&
     isCompound() == rhs.isCompound() &&
     isArray() == rhs.isArray() &&
     fixedSize() == rhs.fixedSize() &&
     isStruct() == rhs.isStruct() &&
     getSize() == rhs.getSize())
    return true;
  return false;
}

void Type::initDefaultTypes()
{
  //initialize POD types and their aliases
  registerType(new PodType("s8", 1, INTEGER, true));
  registerType(new PodType("s16", 2, INTEGER, true));
  registerType(new PodType("s32", 4, INTEGER, true));
  registerType(new PodType("s64", 8, INTEGER, true));
  registerType(new PodType("u8", 1, INTEGER, false));
  registerType(new PodType("u16", 2, INTEGER, false));
  registerType(new PodType("u32", 4, INTEGER, false));
  registerType(new PodType("u64", 8, INTEGER, false));
  //C-style integer aliases
  registerType(new AliasType("char", typeFromName("s8")));
  registerType(new AliasType("short", typeFromName("s16")));
  registerType(new AliasType("int", typeFromName("s32")));
  registerType(new AliasType("long", typeFromName("s64")));
  registerType(new AliasType("uchar", typeFromName("u8")));
  registerType(new AliasType("ushort", typeFromName("u16")));
  registerType(new AliasType("uint", typeFromName("u32")));
  registerType(new AliasType("ulong", typeFromName("u64")));
}

/* Type */
Type* Type::typeFromName(string name)
{
  for(Type* t : Type::types)
  {
    if(t->name == name)
      return t;
  }
  return nullptr;
}

bool Type::isPOD()
{
  return false;
}

bool Type::isArray()
{
  return false;
}

bool Type::isAlias()
{
  return false;
}

/* Plain Old Data (PodType) */
PodType::PodType(string name, int size, PodCategory category, bool sign) : Type(name)
{
  this->size = size;
  this->category = category;
  this->sign = sign;
}

bool PodType::isPOD()
{
  return true;
}

bool PodType::fixedSize()
{
  return true;
}

int PodType::getSize()
{
  return size;
}

/* Struct Type */
StructType::StructType(string name, vector<string> members) : Type(name)
{
  for(string& mem : members)
  {
    Type* t = Type::typeFromName(mem);
    if(!t)
    {
      
    }
    this->members.push_back(t);
  }
} 

bool StructType::fixedSize()
{
  //Fixed if and only if all constituents are fixed size
  bool fixed = true;
}

/* Array Type */
ArrayType::ArrayType(Type* elems) : Type(mangleName(elems->name))
{
  this->elems = elems;
}

static bool ArrayType::fixedSize()
{
  //always false, a fixed size array is a completely different class
  return false;
}

int ArrayType::getSize()
{
  return 0;
}

static string mangleName(string elemType)
{
  return elemType + "_Array";
}

/* Fixed Array Type */
FixedArrayType::FixedArrayType(string name, Type* elems, int n) : Type(mangleNameFixed(elems->name, n))
{
  this->elems = elems;
  this->n = n;
}

bool FixedArrayType::fixedSize()
{
  return elems->fixedSize();
}

int FixedArrayType::getSize()
{
  //if elems->getSize is 0 (not known @ compile time), this is still correct
  return n * elems->getSize();
}

string FixedArrayType::mangleNameFixed(string elemType, int n)
{
  return elemType + "_Array_" + to_string(n);
}

/* Alias Type */
AliasType::AliasType(string name, Type* actual) : Type(name)
{
  this->actual = actual;
}

bool Alias::isPOD()
{
  return actual->isPOD();
}

bool Alias::isCompound()
{
  return actual->isCompound();
}

bool Alias::isArray()
{
  return actual->isArray();
}

bool Alias::fixedSize()
{
  return actual->fixedSize();
}

int Alias::getSize()
{
  return actual->getSize();
}

bool Alias::isAlias()
{
  return true;
}

