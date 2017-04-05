#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include "Parser.hpp"

/**************************
*   Type System Structs   *
**************************/

struct Scope;

struct Type
{
  //Get unique, mangled name for use in backend
  virtual string getCName() = 0;
  Scope* enclosing;
  int dims;
  static void createTypeTable(Parser::ModuleDef& globalModule);
  static Type& getType(string localName, Scope* usedScope);
  static Type& getType(Parser::Member& localName, Scope* usedScope);
  static vector<AP(Type)> table;
};

struct StructType : public Type
{
  StructType(Parser::StructDecl& sd);
  string getCName();
  string name;
  //check for member functions
  //note: self doesn't count as an argument but it is the 1st arg internally
  bool hasFunc(ProcType& type);
  bool hasProc(ProcType& type);
  vector<Trait*> traits;
  vector<Type*> members;
  vector<bool> composed;  //1-1 correspondence with members
};

struct TupleType : public Type
{
  TupleType(Parser::TupleType& tt);
  string getCName();
  vector<Type*> members;
};

struct AliasType : public Type
{
  AliasType(string newName, Type* t);
  AliasType(Parser::Typedef& td);
  string getCName();
  Type* actual;
};

struct EnumType : public Type
{
  EnumType(Parser::Enum& e);
  string getCName();
  string name;
  map<string, int> values;
};

struct IntegerType : public Type
{
  IntegerType(string name, int size, bool sign);
  string getCName();
  //Size in bytes
  string name;
  int size;
  bool isSigned;
};

//A 1-dimensional array of underlying
//Multidimensional arrays are nested
//Makes semantic checking of indexing easier
struct ArrayType : public Type
{
  //constructor also creates Types for all lower dimensions
  ArrayType(Type* t, int dims);
  string getCName();
  Type* underlying;
};

struct FloatType : public Type
{
  FloatType(string name, int size);
  //4 or 8
  string name;
  int size;
  string getCName();
};

struct StringType : public Type
{
  string getCName();
};

#endif

