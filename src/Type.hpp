#ifndef TYPE_H
#define TYPE_H

#include "Misc.hpp"
#include "Utils.hpp"

enum PodCategory
{
  INTEGER,
  FLOATING
};

struct Type
{
  Type(string name);
  static vector<Type*> table;
  static void initDefaultTypes();
  static void registerType(Type* t);
  static bool typeExists(string name);  //can't have two types of different names, but redundant re-declaration of type OK
  static Type* typeFromName(string name);
  //checks everything (name, size, composition, ordering of members)
  virtual bool isPOD() const;
  virtual bool isStruct() const;
  virtual bool isArray() const;
  virtual bool isAlias() const;
  virtual bool fixedSize() const;
  virtual int getSize() const;     //returns 0 if not fixed size
  string name;
  int size;       //size (if it is known at compile-time)
};

bool operator==(const Type& lhs, const Type& rhs);
bool operator!=(const Type& lhs, const Type& rhs);

//Compound types defined in terms of index into global type table

//Plain-old data (just ints, floats, bool)
struct PodType : public Type
{
  PodType(string name, int size, PodCategory category, bool sign);
  bool isPOD() const;
  bool fixedSize() const;
  int getSize() const;
  int size;
  PodCategory category;
  bool sign;
  string getCppName();
};

struct StructType : public Type
{
  StructType(string name, vector<string>& memTypes, vector<string>& memNames);
  vector<Type*> memTypes;
  vector<string> memNames;
  bool isStruct() const;
  bool fixedSize() const;
  int getSize() const;
};

//Array type names are mangled as: 
struct ArrayType : public Type
{
  ArrayType(Type* elems);
  static string mangleName(string elemType);
  virtual bool fixedSize() const;
  virtual int getSize() const;
  Type* elems;
  //TODO: support multi-dim arrays
  int dims;
};

struct FixedArrayType : public Type
{
  FixedArrayType(string name, Type* elems, int n); 
  static string mangleNameFixed(string elemType, int n);
  bool fixedSize() const;
  int getSize() const;
  Type* elems;
  int n;
  int dims;
};

//Registered as initial alias (i.e. s8 vs. char) or at typedef
struct AliasType : public Type
{
  AliasType(string name, Type* actual);
  bool isAlias() const;
  bool isPOD() const;
  bool isStruct() const;
  bool isArray() const;
  bool fixedSize() const;
  int getSize() const;
  Type* actual;
};

#endif
