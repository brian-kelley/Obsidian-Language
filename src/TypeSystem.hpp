#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include <string>
#include <iostream>
#include <vector>
#include "Parser.hpp"

/**************************
*    Middle-End Scope     *
**************************/

enum struct ScopeType
{
  MODULE,
  STRUCT,
  BLOCK
};

struct Scope
{
  virtual ScopeType getType() = 0;
};

struct ModuleScope : public Scope
{
  ScopeType getType();
}

struct StructScope : public Scope
{
  ScopeType getType();
}

struct BlockScope : public Scope
{
  ScopeType getType();
}

/**************************
*   Type System Structs   *
**************************/

struct Type
{
  //Get unique, mangled name for use in backend
  virtual string getCName() = 0;
  Scope* enclosing;
  int dims;
  static vector<Type*> table;
};

struct StructType : public Type
{
  string getCName();
  vector<Type*> members;
};

struct TupleType : public Type
{
  string getCName();
  vector<Type*> members;
};

struct AliasType : public Type
{
  string getCName();
  Type* actual;
};

struct IntegerType : public Type
{
  //Size in bytes
  int size;
  bool isSigned;
  string getCName();
};

//A 1-dimensional array of underlying
//Multidimensional arrays are nested
//Makes semantic checking of indexing easier
struct ArrayType : public Type
{
  string getCName();
  Type* underlying;
};

struct FloatType : public Type
{
  //4 or 8
  int size;
  string getCName();
};

struct String : public Type
{
  string getCName();
};

/**************************
*  Type System Utilities  *
**************************/

//Add a new type to the type table
void createTypeTable(Parser::ModuleDef& globalModule);

//Get the type table entry, given the local usage name and current scope
Type* getType(string localName, Scope* usedScope);
Type* getType(Parser::Member& localName, Scope* usedScope);

#endif

