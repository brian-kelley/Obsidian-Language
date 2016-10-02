#include "Type.hpp"

vector<Type*> Type::table;

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

bool operator==(const Type& lhs, const Type& rhs)
{
  return
     lhs.name == rhs.name &&
     lhs.isPOD() == rhs.isPOD() &&
     lhs.isStruct() == rhs.isStruct() &&
     lhs.isArray() == rhs.isArray() &&
     lhs.fixedSize() == rhs.fixedSize() &&
     lhs.getSize() == rhs.getSize();
}

bool operator!=(const Type& lhs, const Type& rhs)
{
  return !(lhs == rhs);
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
  registerType(new AliasType("bool", typeFromName("u8")));
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
Type::Type(string name)
{
  this->name = name;
}

bool Type::isPOD() const
{
  return false;
}

bool Type::isArray() const
{
  return false;
}

bool Type::isStruct() const
{
  return false;
}

bool Type::isAlias() const
{
  return false;
}

bool Type::fixedSize() const
{
  return false;
}

int Type::getSize() const
{
  return 0;
}

/* Plain Old Data (PodType) */
PodType::PodType(string name, int size, PodCategory category, bool sign) : Type(name)
{
  this->size = size;
  this->category = category;
  this->sign = sign;
}

bool PodType::isPOD() const
{
  return true;
}

bool PodType::fixedSize() const
{
  return true;
}

int PodType::getSize() const
{
  return size;
}

string PodType::getCppName()
{
  if(category == FLOATING)
  {
    if(size == 4)
      return "float";
    else if(size == 8)
      return "double";
    else
      errAndQuit("Only float (32 bit) and double (64 bit) floating point types supported.");
  }
  string intStem;
  switch(size)
  {
    case 1:
      intStem = "char";
      break;
    case 2:
      intStem = "short";
      break;
    case 4:
      intStem = "int";
      break;
    case 8:
      intStem = "long";
      break;
    default:
      errAndQuit("Only 1, 2, 4, and 8 byte integer types supporeted.");
  }
  if(!sign)
  {
    //unsigned type
    return string("u") + intStem;
  }
  else
  {
    return intStem;
  }
}

/* Struct Type */
StructType::StructType(string name, vector<string>& memTypes, vector<string>& memNames) : Type(name)
{
  //First, make sure none of the member type names match this type
  for(string& mem : memTypes)
  {
    if(mem == name)
      errAndQuit(string("Error: Struct \"") + name + "\" has itself as a member (recursive types not allowed).");
  }
  for(size_t i = 0; i < memNames.size(); i++)
  {
    Type* t = Type::typeFromName(memNames[i]);
    if(!t)
    {
      errAndQuit(string("Error: Struct \"") + name + "\" has member of type \"" + memNames[i] + "\" but that type has not yet been defined.");
    }
    this->memTypes.push_back(t);
    this->memNames.push_back(memNames[i]);
  }
} 

bool StructType::fixedSize() const
{
  //Fixed if and only if all constituents are fixed size
  bool fixed = true;
  for(Type* sub : memTypes)
  {
    if(!sub->fixedSize())
    {
      fixed = false;
      break;
    }
  }
  return fixed;
}

int StructType::getSize() const
{
  if(fixedSize())
  {
    int size = 0;
    for(Type* mem : memTypes)
    {
      size += mem->getSize();
    }
    return size;
  }
  else
  {
    return 0;
  }
}

bool StructType::isStruct() const
{
  return true;
}

/* Array Type */
ArrayType::ArrayType(Type* elems) : Type(mangleName(elems->name))
{
  this->elems = elems;
}

bool ArrayType::fixedSize() const
{
  //always false, a fixed size array is a completely different class
  return false;
}

int ArrayType::getSize() const
{
  return 0;
}

string ArrayType::mangleName(string elemType)
{
  return elemType + "_Array";
}

/* Fixed Array Type */
FixedArrayType::FixedArrayType(string name, Type* elems, int n) : Type(mangleNameFixed(elems->name, n))
{
  this->elems = elems;
  this->n = n;
  this->dims = 1;
}

bool FixedArrayType::fixedSize() const
{
  return elems->fixedSize();
}

int FixedArrayType::getSize() const
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

bool AliasType::isPOD() const
{
  return actual->isPOD();
}

bool AliasType::isStruct() const
{
  return actual->isStruct();
}

bool AliasType::isArray() const
{
  return actual->isArray();
}

bool AliasType::fixedSize() const
{
  return actual->fixedSize();
}

int AliasType::getSize() const
{
  return actual->getSize();
}

bool AliasType::isAlias() const
{
  return true;
}

