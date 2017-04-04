#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include <string>
#include <iostream>
#include <vector>

//MiddleEnd scope
struct Scope;

struct Type
{
  virtual string getMangledName() = 0;
  Scope* enclosing;
  int dims;
};

struct StructType : public Type
{
  string getMangledName();
  vector<Type*> members;
};

struct TupleType : public Type
{
  string getMangledName();
  vector<Type*> members;
};

struct AliasType : public Type
{
  string getMangledName();
  Type* actual;
};

struct IntegerType : public Type
{
};

struct FloatType : public Type
{
};

struct String : public Type
{
};

#endif

