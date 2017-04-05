#include "TypeSystem.hpp"

using namespace std;
using namespace Parser;

vector<AP(Type)> Type::table;

/***********************/
/* Type and subclasses */
/***********************/

void Type::createBuiltinTypes();
{
  //first, add primitives and their aliases
  table.push_back(new IntegerType("char", 1, true));
  table.push_back(new AliasType("i8", &table.back());
  table.push_back(new IntegerType("uchar", 1, false));
  table.push_back(new AliasType("u8", &table.back());
  table.push_back(new IntegerType("short", 2, true));
  table.push_back(new AliasType("i16", &table.back());
  table.push_back(new IntegerType("ushort", 2, false));
  table.push_back(new AliasType("u16", &table.back());
  table.push_back(new IntegerType("int", 4, true));
  table.push_back(new AliasType("i32", &table.back());
  table.push_back(new IntegerType("uint", 4, false));
  table.push_back(new AliasType("u32", &table.back());
  table.push_back(new IntegerType("long", 8, true));
  table.push_back(new AliasType("i64", &table.back());
  table.push_back(new IntegerType("ulong", 8, false));
  table.push_back(new AliasType("u64", &table.back());
  table.push_back(new FloatType("float", 4));
  table.push_back(new AliasType("f32", &table.back());
  table.push_back(new FloatType("double", 8));
  table.push_back(new AliasType("f64", &table.back());
  table.push_back(new StringType);
}

//Get the type table entry, given the local usage name and current scope
Type* Type::getType(string localName, Scope* usedScope)
{
}

Type* Type::getType(Parser::Member& localName, Scope* usedScope)
{
}

StructType::StructType(StructDecl& sd)
{
  name = sd.name;
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
    //todo: support larger-precision ints
  }
}

ArrayType::ArrayType(Type* t, int dims)
{
}

string ArrayType::getCName()
{
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
  else
  {
    //TODO (low-pri): support larger-precision software floats?
    return "ERROR";
  }
}

string StringType::getCName()
{
  return "string";
}

