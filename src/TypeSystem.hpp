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
  Type(Scope* enclosingScope, int arrayDims);
  //Get unique, mangled name for use in backend
  static void createBuiltinTypes();
  virtual string getCName() = 0;
  Scope* enclosing;
  int dims;
  //Functions used by semantic checking
  static Type& getType(string localName, Scope* usedScope);
  static Type& getType(Parser::Member& localName, Scope* usedScope);
  static vector<AP(Type)> table;
};

struct StructType : public Type
{
  StructType(Parser::StructDecl& sd, Scope* enclosingScope);
  string getCName();
  string name;
  //check for member functions
  //note: self doesn't count as an argument but it is the 1st arg internally
  bool hasFunc(ProcType& type);
  bool hasProc(ProcType& type);
  vector<AP(Trait)> traits;
  vector<AP(Type)> members;
  vector<bool> composed;  //1-1 correspondence with members
};

struct TupleType : public Type
{
  //TupleType has no scope, all are global
  TupleType(Parser::TupleType& tt);
  string getCName();
  vector<Type*> members;
};

struct AliasType : public Type
{
  AliasType(string newName, Type* t, Scope* enclosingScope);
  AliasType(Parser::Typedef& td, Scope* enclosingScope);
  string getCName();
  Type* actual;
};

struct EnumType : public Type
{
  EnumType(Parser::Enum& e, Scope* enclosingScope);
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

//An UndefType is a placeholder in Scope's type list.
//Create one for
struct UndefType : public Type
{
  UndefType();
  string getCName();
  static int num;
}

#endif

