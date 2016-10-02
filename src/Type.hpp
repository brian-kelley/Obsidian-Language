#ifndef TYPE_H
#define TYPE_H

enum struct PodCategory
{
  INTEGER,
  FLOATING
}

void initDefaultTypes();

struct Type
{
  Type(string name);
  static vector<Type*> table;
  static void registerType(Type* t);
  static bool typeExists(string name);  //can't have two types of different names, but redundant re-declaration of type OK
  static Type* typeFromName(string name);
  //checks everything (name, size, composition, ordering of members)
  virtual bool operator==(const Type& rhs) = 0;
  virtual bool isPOD();
  virtual bool isCompound();
  virtual bool isArray();
  virtual bool isAlias();
  virtual bool fixedSize();
  virtual int getSize();      //returns 0 if not fixed size
  string name;
  int size;       //size (if it is known at compile-time)
};

//Compound types defined in terms of index into global type table

//Plain-old data (just ints, floats, bool)
struct PodType : public Type
{
  PodType(string name, int size, PodCategory category, bool sign);
  bool isPOD();
  bool fixedSize();
  int getSize();
  int size;
  PodCategory category;
  bool sign;
};

struct StructType : public Type
{
  StructType(string name, vector<string> members);
  vector<Type*> members;
  bool isFixedSize();
  int getSize();
};

//Array type names are mangled as: 
struct ArrayType : public Type
{
  ArrayType(Type* elems);
  static string mangleName(string elemType);
  virtual bool fixedSize();
  virtual int getSize();
  Type* elems;
}

struct FixedArrayType : public Type
{
  FixedArrayType(string name, Type* elems, int n); 
  static string mangleNameFixed(string elemType, int n);
  bool fixedSize();
  int getSize();
  Type* elems;
  int n;
};

//Registered as initial alias (i.e. s8 vs. char) or at typedef
struct AliasType : public Type
{
  AliasType(string name, Type* actual);
  bool isAlias();
  bool isPOD();
  bool isCompound();
  bool isArray();
  bool fixedSize();
  int getSize();
  Type* actual;
}

#endif
