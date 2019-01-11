#include "C_Backend.hpp"
#include <utility>

using namespace IR;
using std::pair;

namespace C
{
  //This increments each time a new C identifier is needed
  int identCount = 0;

  //A table of all non-primitive types in use
  set<CType*, CTypeCompare> allTypes;
  //There is only 1 CType for all unions
  CUnion cUnion;
  map<Type*, CPrim> cprims;

  map<Type*, string> types;
  map<Type*, bool> typesImplemented;
  map<Subroutine*, string> subrs;
  map<Variable*, string> vars;

  //The sets of types where util functions have been implemented
  set<Type*> initImpl;
  set<Type*> copyImpl;
  set<Type*> allocImpl;
  set<Type*> deallocImpl;
  set<Type*> printImpl;
  set<Type*> hashImpl;
  set<pair<Type*, Type*>> convertImpl;
  set<Type*> equalsImpl;
  set<Type*> lessImpl;
  set<ArrayType*> concatImpl;
  set<ArrayType*> prependImpl;
  set<ArrayType*> appendImpl;
  set<Type*> sortImpl;
  set<ArrayType*> accessImpl;
  set<ArrayType*> assignImpl;

  bool operator==(const CPrim& p1, const CPrim& p2)
  {
    return p1->type == p2->type;
  }
  bool operator<(const CPrim& p1, const CPrim& p2)
  {
    return p1->type < p2->type;
  }

  CStruct::CStruct(StructType* st)
  {
    for(size_t i = 0; i < st->members.size(); i++)
    {
      mems.push_back(getCType(st->members[i]->type));
    }
    id = identCount++;
  }

  CStruct::CStruct(TupleType* tt)
  {
    for(size_t i = 0; i < tt->members.size(); i++)
    {
      mems.push_back(getCType(tt->members[i]));
    }
    id = identCount++;
  }

  bool operator==(const CStruct& s1, const CStruct& s2)
  {
    if(s1->mems.size() != s2->mems.size())
      return false;
    return std::equal(s1->mems.begin(), s1->mems.end(), s2->mems.begin(), CTypeEqual());
  }
  bool operator<(const CStruct& s1, const CStruct& s2)
  {
    return std::lexicographical_compare(s1->mems.begin(), s1->mems.end(),
        s2->mems.begin(), s2->mems.end(), CTypeCompare());
  }

  CArray::CArray(ArrayType* at)
  {
    subtype = getCType(at->subtype);
    id = identCount++;
  }

  bool operator==(const CArray& a1, const CArray& a2)
  {
    return CTypeEqual()(a1->subtype, a2->subtype);
  }
  bool operator<(const CArray& a1, const CArray& a2)
  {
    return CTypeCompare()(a1->subtype, a2->subtype);
  }

  CMap::CMap(MapType* mt)
  {
    key = getCType(mt->key);
    value = getCType(mt->value);
    id = identCount++;
  }

  bool operator==(const CMap& m1, const CMap& m2)
  {
    CTypeEqual compare;
    return compare(m1->key, m2->key) && compare(m1->value, m2->value);
  }
  bool operator<(const CMap& m1, const CMap& m2)
  {
    CTypeEqual equal;
    CTypeCompare compare;
    if(compare(m1->key, m2->key))
      return true;
    if(!equal(m1->key, m2->key))
      return false;
    //keys same, now compare values
    return compare(m1->value, m2->value);
  }

  CCallable::CCallable(CallableType* ct)
  {
    init(ct);
  }
  CCallable::CCallable(Subroutine* subr)
  {
    init(subr->type);
  }
  void CCallable::init(CallableType* ct)
  {
    if(ct->ownerStruct)
    {
      thisType = dynamic_cast<CStruct*>(getCType(ct->ownerStruct));
      INTERNAL_ASSERT(thisType);
    }
    returnType = getCType(ct->returnType);
    for(auto arg : ct->argTypes)
    {
      CType* argType = getCType(arg);
      if(!argType->isVoid())
        args.push_back(argType);
    }
  }

  bool operator==(const CCallable& c1, const CCallable& c2)
  {
    CTypeEqual equal;
    bool method1 = c1.thisType == nullptr;
    bool method2 = c2.thisType == nullptr;
    if(method1 != method2)
      return false;
    if(c1.args.size() != c2.args.size())
      return false;
    if(method1 && *c1.thisType != *c2.thisType)
      return false;
    if(!equal(c1.returnType, c2.returnType))
      return false;
    for(size_t i = 0; i < c1.args.size(); i++)
    {
      if(!equal(c1.args[i], c2.args[i]))
        return false;
    }
    return true;
  }
  bool operator<(const CCallable& c1, const CCallable& c2)
  {
    CTypeEqual equal;
    CTypeCompare compare;
    if(c1.args.size() < c2.args.size())
      return true;
    if(c1.args.size() > c2.args.size())
      return false;
    //static < method
    bool method1 = c1.thisType == nullptr;
    bool method2 = c2.thisType == nullptr;
    if(method1 != method2)
      return false;
    if(!method1 && method2)
      return true;
    else if(method1 && !method2)
      return false;
    if(method1)
    {
      if(*c1.thisType < *c2.thisType)
        return true;
      else if(!(*c1.thisType == *c2.thisType))
        return false;
    }
    if(compare(c1.returnType, c2.returnType))
      return true;
    else if(!equal(c1.returnType, c2.returnType))
      return false;
    for(size_t i = 0; i < c1.args.size(); i++)
    {
      if(compare(c1.args[i], c2.args[i]))
        return true;
      else if(!equal(c1.args[i], c2.args[i]))
        return false;
    }
    //c1, c2 identical
    return false;
  }

  bool CTypeCompare::operator()(const CType* t1, const CType* t2)
  {
    if(t1->getKindTag() < t2->getKindTag())
      return true;
    else if(t1->getKindTag() > t2->getKindTag())
      return false;
    //t1 and t2 are the same kind of type
    if(auto p1 = dynamic_cast<CPrim*>(t1))
    {
      auto p2 = dynamic_cast<CPrim*>(t2);
      return *p1 < *p2;
    }
    else if(auto s1 = dynamic_cast<CStruct*>(t1))
    {
      auto s2 = dynamic_cast<CStruct*>(t2);
      return *s1 < *s2;
    }
    else if(auto a1 = dynamic_cast<CArray*>(t1))
    {
      auto a2 = dynamic_cast<CArray*>(t2);
      return *a1 < *a2;
    }
    else if(dynamic_cast<CUnion*>(t1))
    {
      return false;
    }
    else if(auto m1 = dynamic_cast<CMap*>(t1))
    {
      auto m2 = dynamic_cast<CMap*>(t2);
      return *m1 < *m2;
    }
    INTERNAL_ERROR;
    return false;
  }

  bool CTypeEqual::operator()(const CType* t1, const CType* t2)
  {
    if(t1->getKindTag() != t2->getKindTag())
      return false;
    if(auto p1 = dynamic_cast<CPrim*>(t1))
    {
      auto p2 = dynamic_cast<CPrim*>(t2);
      return *p1 == *p2;
    }
    else if(auto s1 = dynamic_cast<CStruct*>(t1))
    {
      auto s2 = dynamic_cast<CStruct*>(t2);
      return *s1 == *s2;
    }
    else if(auto a1 = dynamic_cast<CArray*>(t1))
    {
      auto a2 = dynamic_cast<CArray*>(t2);
      return *a1 == *a2;
    }
    else if(dynamic_cast<CUnion*>(t1))
    {
      return true;
    }
    else if(auto m1 = dynamic_cast<CMap*>(t1))
    {
      auto m2 = dynamic_cast<CMap*>(t2);
      return *m1 == *m2;
    }
    INTERNAL_ERROR;
    return false;
  }

  CType* getCType(Type* t)
  {
    t = canonicalize(t);
    //Primitives and unions all use predefined C types
    //Onyx primitives map 1-1 to C primitives
    if(t->isPrimitive())
    {
      //All simple types have 0-size representation (void)
      if(dynamic_cast<SimpleType*>(t))
        return &cprims[primitives[Prim::VOID]];
      else if(auto et = dynamic_cast<EnumType*>(t))
        t = et->underlying;
      INTERNAL_ASSERT(cprims.find(t) != cprims.end());
      return &cprims[t];
    }
    //All unions represented by this type
    if(t->isUnion())
      return &cUnion;
    //Create the compound type
    //If it's already in allTypes, return that
    //otherwise add it to allTypes
    CType* compound = nullptr;
    if(auto st = dynamic_cast<StructType*>(t))
      compound = new CStruct(st);
    else if(auto tt = dynamic_cast<TupleType*>(t))
      compound = new CStruct(tt);
    else if(auto at = dynamic_cast<ArrayType*>(t))
      compound = new CArray(at->subtype);
    else if(auto mt = dynamic_cast<MapType*>(t))
      compound = new CMap(mt);
    else if(auto ct = dynamic_cast<CallableType*>(t))
      compound = new CCallable(ct);
    INTERNAL_ASSERT(compound);
    //now, look up the compound type in allTypes
    auto it = allTypes.find(compound);
    if(it == allTypes.end())
    {
      //it's a new type
      allTypes.insert(compound);
      return compound;
    }
    //already have it, can safely delete the newly created one
    delete compound;
    return *it;
  }

  string getTypeName(Type* t)
  {
    return getCType(t)->getName();
  }

  string getTypeName(CType* c)
  {
    return c->getName();
  }

  struct Temporary
  {
    Temporary(Type* t, string name) : type(t), cname(name) {}
    //emit any code necessary to deallocate memory owned by this
    void emitFree(ostream& c)
    {
      if(typeNeedsDealloc(type))
      {
        c << getDeallocFunc(type) << '(' << cname << ");";
      }
    }
    Type* type;
    string cname;
  };

  ofstream c;
  //different stringstreams to build the C file (in this order)
  Oss typeDecls;      //all typedefs (including forward-declarations as necessary)
  Oss varDecls;       //all global/static variables
  Oss utilFuncDecls;  //util functions are print, array creation, deep copy, etc
  Oss utilFuncDefs;
  Oss funcDecls;      //the actual subroutines in the onyx program
  Oss funcDefs;

  namespace Context
  {
    map<Switch*, string> switchBreakLabels;
    map<For*, string> forBreakLabels;
    map<While*, string> whileBreakLabels;
  }

  //The C type to use for array sizes/indices
  const char* size_type = "int32_t";

  void init()
  {
    cprims[primitives[BOOL]] = CPrim(CPrim::BOOL);
    cprims[primitives[CHAR]] = CPrim(CPrim::I8);
    cprims[primitives[BYTE]] = CPrim(CPrim::I8);
    cprims[primitives[UBYTE]] = CPrim(CPrim::U8);
    cprims[primitives[SHORT]] = CPrim(CPrim::I16);
    cprims[primitives[USHORT]] = CPrim(CPrim::U16);
    cprims[primitives[INT]] = CPrim(CPrim::I32);
    cprims[primitives[UINT]] = CPrim(CPrim::U32);
    cprims[primitives[LONG]] = CPrim(CPrim::I64);
    cprims[primitives[ULONG]] = CPrim(CPrim::U64);
    cprims[primitives[FLOAT]] = CPrim(CPrim::F32);
    cprims[primitives[DOUBLE]] = CPrim(CPrim::F64);
    cprims[primitives[VOID]] = CPrim(CPrim::VOID);
    cprims[primitives[ERROR]] = CPrim(CPrim::VOID);
  }

  void generate(string outputStem, bool keep)
  {
    init();
    cout << "Starting C backend.\n";
    string cName = outputStem + ".c";
    string exeName = outputStem + ".exe";
    typeDecls = Oss();
    varDecls = Oss();
    utilFuncDecls = Oss();
    utilFuncDefs = Oss();
    funcDecls = Oss();
    funcDefs = Oss();
    generateSectionHeader(utilFuncDecls, "Internal functions");
    cout << "  > Generating type declarations\n";
    generateSectionHeader(typeDecls, "Type Decls");
    genTypeDecls();
    cout << "  > Generating global variables\n";
    generateSectionHeader(varDecls, "Global Variables");
    //create the global scope, which will be popped at the end of main()
    //pushScope();
    genGlobals();
    cout << "  > Generating Onyx subroutines\n";
    generateSectionHeader(funcDecls, "Subroutine declarations");
    generateSectionHeader(funcDefs, "Subroutine definitions");
    genSubroutines();
    c = ofstream(cName);
    c << "//--- " << outputStem << ".c, generated by the Onyx Compiler ---//\n\n";
    //genCommon and implHashTable write directly to the main stream
    genCommon();
    implHashTable();
    //write types, vars, func decls, func defs in the ostringstreams
    c.write(typeDecls.str().c_str(), typeDecls.tellp());
    c << "\n";
    c.write(varDecls.str().c_str(), varDecls.tellp());
    c << "\n";
    c.write(utilFuncDecls.str().c_str(), utilFuncDecls.tellp());
    c << "\n";
    c.write(utilFuncDefs.str().c_str(), utilFuncDefs.tellp());
    c.write(funcDecls.str().c_str(), funcDecls.tellp());
    c << "\n";
    c.write(funcDefs.str().c_str(), funcDefs.tellp());
    c << '\n';
    cout << "  > Done, wrote " << c.tellp() << " bytes of C source code\n";
    c.close();
    //wait for cc to terminate
    bool smallBin = false;
    bool compileSuccess = false;
    //Common flags for C compiler
    string cflags = "-Wall -Wextra -Werror --std=c99 -ffast-math -fassociative-math -o " + exeName + ' ' + cName;
    if(smallBin)
    {
      compileSuccess = runCommand("gcc -Os " + cflags + " &> cc.out");
      //shrink binary some more (no need for symbol names)
      if(compileSuccess)
      {
        //stdout from this command is silenced
        runCommand(string("gstrip --strip-all ") + exeName, true);
      }
    }
    else
    {
      compileSuccess = runCommand("gcc -g " + cflags + " &> cc.out");
    }
    if(!keep)
    {
      remove(cName.c_str());
    }
    if(!compileSuccess)
    {
      errMsg("C compiler encountered error.");
    }
  }

  void genCommon()
  {
    c << "#include \"stdio.h\"\n";
    c << "#include \"stdlib.h\"\n";
    c << "#include \"math.h\"\n";
    c << "#include \"string.h\"\n";
    c << "#include \"stdint.h\"\n";
    c << "#include \"stdbool.h\"\n\n";
    c << "void panic(const char* why_)\n";
    c << "{\n";
    c << "fprintf(stderr, \"%s\\n\", why_);\n";
    c << "exit(1);\n";
    c << "}\n\n";
  }

  void implHashTable()
  {
    //each hash table entry contains key ptr, value ptr and hash
    c <<
      //Bucket for hash table: basically std::vector<void*>
      //also stores the hashes for each key so table expansion is faster
      "\ntypedef struct\n"
      "{\n"
      "void** keys;\n"
      "void** values;\n"
      "uint32_t* hashes;\n"
      "int size;\n"
      "int cap;\n"
      "} Bucket;\n\n"
      //Hash table: all (x : y) types are implemented as this
      "typedef struct\n"
      "{\n"
      "uint32_t (*hashFn)(void* data);\n"
      "bool (*compareFn)(void* lhs, void* rhs);\n"
      "Bucket* buckets;\n"
      "int size;\n"         //size = number of key-value pairs
      "int numBuckets;\n"   //log2(actual number of buckets)
      "} HashTable;\n\n"
      //Insert (key, value, h(key)) into a bucket, expanding it as necessary
      "void bucketInsert(Bucket* bucket, void* key, void* data, uint32_t hash)\n{\n"
      "if(bucket->size == bucket->cap)\n"
      "{\n"
      "int newCap = (bucket->cap == 0) ? 1 : bucket->cap * 2;\n"
      "bucket->keys = realloc(bucket->keys, newCap * sizeof(void*));\n"
      "bucket->values = realloc(bucket->values, newCap * sizeof(void*));\n"
      "bucket->hashes = realloc(bucket->hashes, newCap * sizeof(uint32_t*));\n"
      "bucket->cap = newCap;\n"
      "}\n"
      "bucket->keys[bucket->size] = key;\n"
      "bucket->values[bucket->size] = data;\n"
      "bucket->hashes[bucket->size] = hash;\n"
      "bucket->size++;\n"
      "}\n\n"
      //Remove given key from bucket (if it exists), then shift down remaining elements to fill space
      //if bucket now contains 0 things, free its arrays
      "void bucketRemove(Bucket* bucket, void* key, bool (*compareFn)(void* lhs, void* rhs))\n"
      "{\n"
      "for(int i = 0; i < bucket->size; i++)\n"
      "{\n"
      "if(compareFn(bucket->keys[i], key))\n"
      "{\n"
      "//found it, shift all the arrays down 1 element\n"
      "memmove(bucket->keys + i, bucket->keys + i + 1, sizeof(void*) * (bucket->size - i - 1));\n"
      "memmove(bucket->values + i, bucket->values + i + 1, sizeof(void*) * (bucket->size - i - 1));\n"
      "memmove(bucket->hashes + i, bucket->hashes + i + 1, sizeof(uint32_t) * (bucket->size - i - 1));\n"
      "bucket->size--;\n"
      "if(bucket->size == 0)\n"
      "{\n"
      "free(bucket->keys);\n"
      "free(bucket->values);\n"
      "free(bucket->hashes);\n"
      "bucket->cap = 0;\n"
      "}\n"
      "break;\n"
      "}\n"
      "}\n"
      "}\n\n"
      //Find a key in a bucket (return NULL if not found)
      "void* bucketFind(Bucket* bucket, void* key, bool (*compareFn)(void* lhs, void* rhs))\n"
      "{\n"
      "for(int i = 0; i < bucket->size; i++)\n"
      "{\n"
      "if(compareFn(bucket->keys[i], key))\n{\n"
      "return bucket->values[i];\n"
      "}\n}\n"
      "return NULL;\n"
      "}\n\n"
      //Insert item to hash table
      //Double the number of buckets if average items per is > 1
      //This is because comparing for equality is assumed to be expensive
      "void hashInsert(HashTable* table, void* key, void* data)\n{\n"
      "//resize table if necessary\n"
      "if(table->size == table->numBuckets)\n"
      "{\n"
      //nb = new # of buckets
      "int nb = table->numBuckets ? table->numBuckets * 2 : 16;\n"
      "Bucket* newBuckets = calloc(nb, sizeof(Bucket));\n"
      "for(int i = 0; i < table->numBuckets; i++)\n"
      "{\n"
      "Bucket* oldBucket = table->buckets + i;\n"
      "for(int j = 0; j < oldBucket->size; j++)\n"
      "{\n"
      "Bucket* newBucket = newBuckets + (oldBucket->hashes[j] & (nb - 1));\n"
      "bucketInsert(newBucket, oldBucket->keys[j], oldBucket->values[j], oldBucket->hashes[j]);\n"
      "}\n"
      "//free the bucket and its contents\n"
      "free(oldBucket->keys);\n"
      "free(oldBucket->values);\n"
      "free(oldBucket->hashes);\n"
      "}\n"
      "free(table->buckets);\n"
      "table->buckets = newBuckets;\n"
      "table->numBuckets = nb;\n"
      "}\n"
      "uint32_t h = (table->hashFn)(key);\n"
      "bucketInsert(table->buckets + (h & (table->numBuckets - 1)), key, data, h);\n"
      "}\n\n"
      //hash table lookup: NULL if not found
      "void* hashFind(HashTable* table, void* key)\n{\n"
      "return bucketFind(table->buckets + ((table->hashFn)(key) & (table->numBuckets - 1)), key, table->compareFn);\n"
      "}\n\n"
      //hash table remove (if it exists, otherwise no-op)
      "void hashRemove(HashTable* table, void* key)\n{\n"
      "bucketRemove(table->buckets + ((table->hashFn)(key) & (table->numBuckets - 1)), key, table->compareFn);\n"
      "}\n\n";
  }

  void genTypeDecls()
  {
    types[primitives[Prim::BOOL]] = "bool";
    types[primitives[Prim::VOID]] = "char";
    types[primitives[Prim::ERROR]] = "char";
    types[primitives[Prim::CHAR]] = "char";
    types[primitives[Prim::BYTE]] = "int8_t";
    types[primitives[Prim::UBYTE]] = "uint8_t";
    types[primitives[Prim::SHORT]] = "int16_t";
    types[primitives[Prim::USHORT]] = "uint16_t";
    types[primitives[Prim::INT]] = "int32_t";
    types[primitives[Prim::UINT]] = "uint32_t";
    types[primitives[Prim::LONG]] = "int64_t";
    types[primitives[Prim::ULONG]] = "uint64_t";
    types[primitives[Prim::FLOAT]] = "float";
    types[primitives[Prim::DOUBLE]] = "double";
    for(auto prim : primitives)
    {
      typesImplemented[prim] = true;
    }
    //like primitives, enums can be fully implemented immediately
    for(auto et : enums)
    {
      typesImplemented[et] = true;
      //figure out the range of types for the enum,
      //then use the smallest int type (in bytes)
      //that can hold all values
      string enumType;
      if(et->values.size() == 0)
      {
        enumType = "char";
      }
      else
      {
        long long minVal = LLONG_MAX;
        long long maxVal = LLONG_MIN;
        for(auto val : et->values)
        {
          if(val->value < minVal)
            minVal = val->value;
          if(val->value > maxVal)
            maxVal = val->value;
        }
        if(minVal >= 0)
        {
          //unsigned
          if(maxVal < (1LL << 8))
            enumType = "uint8_t";
          else if(maxVal < (1LL << 16))
            enumType = "uint16_t";
          else if(maxVal < (1LL << 32))
            enumType = "uint32_t";
          else
            enumType = "uint64_t";
        }
        else
        {
          //signed (2s complement)
          if(minVal >= -(1LL << 7) && maxVal < (1LL << 7))
            enumType = "int8_t";
          else if(minVal >= -(1LL << 15) && maxVal < (1LL << 15))
            enumType = "int16_t";
          else if(minVal >= -(1LL << 31) && maxVal < (1LL << 31))
            enumType = "int32_t";
          else
            enumType = "int64_t";
        }
      }
      string id = getIdentifier();
      types[et] = id;
      typeDecls << "typedef " << enumType << ' ' << id << ';';
      typeDecls << " //" << et->getName() << '\n';
    }
    //maps are special because they are all just HashTable*
    //(HashTable has already been fully defined above)
    for(auto t : maps)
    {
      string id = getIdentifier();
      types[t] = id;
      typeDecls << "typedef HashTable " << id << "; //" << t->getName() << '\n';
      typesImplemented[t] = true;
    }
    //need to assign names to and forward declare structs
    //for all non-primitive types
    auto forwardDeclare = [&] (Type* t) -> void
    {
      typesImplemented[t] = false;
      string id = getIdentifier();
      types[t] = id;
      //C is annoying so you have to forward-decl the struct, then a typedef using it
      typeDecls << "struct _" << id << ";\n";
      typeDecls << "typedef struct _" << id << ' ' << id << "; //" << t->getName() << '\n';
    };
    for(auto t : structs)
    {
      forwardDeclare(t);
    }
    for(auto t : arrays)
    {
      forwardDeclare(t);
    }
    for(auto t : unions)
    {
      forwardDeclare(t);
    }
    for(auto t : tuples)
    {
      forwardDeclare(t);
    }
    typeDecls << '\n';
    //implement all callable types
    for(auto t : callables)
    {
      CallableType* ct = (CallableType*) t;
      string id = getIdentifier();
      types[t] = id;
      typesImplemented[t] = true;
      typeDecls << "typedef " << types[ct->returnType] << "(*" << id << ")(";
      for(size_t i = 0; i < ct->argTypes.size(); i++)
      {
        if(i != 0)
          typeDecls << ", ";
        Type* argType = ct->argTypes[i];
        typeDecls << types[argType];
        if(!argType->isPrimitive())
        {
          typeDecls << '*';
        }
      }
      typeDecls << "); //" << t->getName() << "\n";
    }
    typeDecls << '\n';
    //implement all compound types
    for(auto at : arrays)
    {
      typeDecls << "struct _" << types[at] << " //" << at->getName() << "\n{\n";
      //Two levels of indirection: data is dim-sized array of single subtype pointers
      //this is inefficient but makes freeing stuff much simpler
      typeDecls << types[at->subtype] << "* data;\n";
      typeDecls << size_type << " dim;\n";
      typeDecls << "};\n\n";
    }
    for(auto st : structs)
    {
      typeDecls << "struct _" << types[st] << " //" << st->getName() << "\n{\n";
      for(size_t i = 0; i < st->members.size(); i++)
      {
        typeDecls << types[st->members[i]->type] << " mem" << i << ";\n";
      }
      typeDecls << "};\n\n";
    }
    for(auto tt : tuples)
    {
      typeDecls << "struct _" << types[tt] << " //" << tt->getName() << "\n{\n";
      for(size_t i = 0; i < tt->members.size(); i++)
      {
        typeDecls << types[tt->members[i]] << " mem" << i << ";\n";
      }
      typeDecls << "};\n\n";
    }
    for(auto ut : unions)
    {
      typeDecls << "struct _" << types[ut] << " //" << ut->getName() << "\n{\n";
      typeDecls << "void* data;\n";
      typeDecls << "int tag;\n";
      typeDecls << "};\n\n";
    }
  }

  bool typeNeedsDealloc(Type* t)
  {
    return t->isArray() || t->isUnion() || t->isMap();
  }

  bool isPOD(Type* t)
  {
    return t->isPrimitive() || t->isEnum();
  }

  bool isPointer(Expression* expr)
  {
    if(dynamic_cast<ThisExpr*>(expr))
      return true;
    else if(VarExpr* ve = dynamic_cast<VarExpr*>(expr))
    {
      //Only non-POD subroutine arguments are accessed as pointers
      if(!isPOD(ve->type) && ve->var->scope->node.is<Subroutine*>())
      {
        return true;
      }
    }
    return false;
  }

  void genGlobals()
  {
    //there should be exactly one C scope on the stack now
    int numGlobals = 0;
    walkScopeTree([&] (Scope* s) -> void
        {
        if(s->node.is<Block*>() || s->node.is<Subroutine*>())
        {
        //don't do local variables now
        return;
        }
        for(auto n : s->names)
        {
        if(n.second.kind != Name::VARIABLE)
        {
        continue;
        }
        Variable* v = (Variable*) n.second.item;
        //struct members have no corresponding variable decl in C
        if(v->owner)
        {
        continue;
        }
        //NOTE: initialization can't be done in C global scope,
        //instead happens at the beginning of main()
        string id = getIdentifier();
        varDecls << types[v->type] << ' ' << id << "; //" << v->type->getName() << ' ' << v->name << '\n';
        vars[v] = id;
        //addScopedVar(v->type, id);
        numGlobals++;
        }
        });
    if(numGlobals)
    {
      varDecls << '\n';
    }
  }

  void genSubroutines()
  {
    walkScopeTree([&] (Scope* s) -> void
        {
        for(auto& n : s->names)
        {
        if(n.second.kind != Name::SUBROUTINE || n.first == "main")
        {
        continue;
        }
        auto sub = (Subroutine*) n.second.item;
        string name = getIdentifier();
        subrs[sub] = name;
        funcDecls << types[sub->type->returnType] << ' ' << name << '(';
        int totalArgs = 0;
        if(sub->type->ownerStruct)
        {
        funcDecls << types[sub->type->ownerStruct] << "* this";
        totalArgs++;
        }
        //open scope for subroutine
        //pushScope();
        for(size_t i = 0; i < sub->args.size(); i++)
        {
          Variable* arg = sub->args[i];
          if(totalArgs > 0)
          {
            funcDecls << ", ";
          }
          totalArgs++;
          string argName = getIdentifier();
          //primitives passed by value, all others passed by ptr
          vars[arg] = argName;
          funcDecls << types[arg->type];
          if(isPOD(arg->type))
            funcDecls << ' ' << argName;
          else
            funcDecls << "* " << argName;
        }
        funcDecls << ");\n";
        }
        });
    walkScopeTree([&] (Scope* s) -> void
        {
        for(auto& n : s->names)
        {
        if(n.second.kind != Name::SUBROUTINE || n.first == "main")
        {
        continue;
        }
        auto sub = (Subroutine*) n.second.item;
        funcDefs << types[sub->type->returnType] << ' ' << subrs[sub] << '(';
        int totalArgs = 0;
        if(sub->type->ownerStruct)
        {
        funcDefs << types[sub->type->ownerStruct] << "* this";
        totalArgs++;
        }
        for(size_t i = 0; i < sub->args.size(); i++)
        {
        auto arg = sub->args[i];
        if(totalArgs > 0)
        {
          funcDefs << ", ";
        }
        totalArgs++;
        string argName = getIdentifier();
        vars[arg] = argName;
        funcDefs << types[arg->type];
        if(arg->type->isPOD())
          funcDefs << ' ' << argName;
        else
          funcDefs << "* " << argName;
        }
        funcDefs << ")\n{\n";
        //generate all statements in the body, in sequence
        for(auto stmt : ir[sub])
        {
          emitStatement(funcDefs, stmt);
        }
        funcDefs << "}\n\n";
        }
        });
    genMain((Subroutine*) global->scope->names["main"].item);
  }

  void genMain(Subroutine* m)
  {
    funcDefs << "int main(";
    if(m->args.size() == 1)
    {
      //single argument: must be string[]
      funcDefs << "int argc, const char** argv)\n{\n";
      //manually allocate the string[] and copy in the args
      //(don't include the first argument)
      Variable* arg = m->args[0];
      vars[arg] = getIdentifier();
      //addScopedVar(stringType, vars[arg]);
      funcDefs << types[arg->type] << ' ' << vars[arg] << ";\n";
      funcDefs << getAllocFunc((ArrayType*) arg->type) << "(&" << vars[arg];
      funcDefs << ", argc - 1, 0);\n";
      funcDefs << "for(int i = 0; i < argc - 1; i++)\n{\n";
      funcDefs << vars[arg] << ".data[i].data = strdup(argv[i]);\n";
      funcDefs << vars[arg] << ".data[i].dim = strlen(argv[i]));\n";
      funcDefs << "}\n";
    }
    else
    {
      funcDefs << ")\n{\n";
    }
    //generate local variables
    generateLocalVariables(funcDefs, m->body);
    //generate all statements like normal (one at a time)
    for(auto stmt : m->body->stmts)
    {
      generateStatement(funcDefs, m->body, stmt);
    }
    //if main was declared void, add "return 0" to avoid warning
    //(because C return type is always int)
    if(m->type->returnType == primitives[Prim::VOID])
    {
      funcDefs << "return 0;\n";
    }
    funcDefs << "}\n";
  }

  string generateExpression(ostream& c, Expression* expr)
  {
    //first, handle special cases where expr is already a simple C lvalue
    if(VarExpr* var = dynamic_cast<VarExpr*>(expr))
    {
      return vars[var->var];
    }
    else if(dynamic_cast<ThisExpr*>(expr))
    {
      return "this";
    }
    //maintain a list of temporaries, so memory can be freed ASAP
    vector<Temporary> temporaries;
    string ident = getIdentifier();
    //declare the output value (uninitialized)
    c << types[expr->type] << ' ' << ident << ";\n";
    //open a scope so stack allocations will go out of scope when not needed
    c << "{ // Evaluating " << expr << '\n';
    //Expressions in C mostly depend on the subclass of expr
    if(UnaryArith* unary = dynamic_cast<UnaryArith*>(expr))
    {
      string operand = generateExpression(c, unary->expr);
      temporaries.emplace_back(unary->expr->type, operand);
      c << ident << " = " << operatorTable[unary->op] << operand << ";\n";
    }
    else if(BinaryArith* binary = dynamic_cast<BinaryArith*>(expr))
    {
      string lhs = generateExpression(c, binary->lhs);
      string rhs = generateExpression(c, binary->rhs);
      temporaries.emplace_back(binary->lhs->type, lhs);
      temporaries.emplace_back(binary->rhs->type, rhs);
      //fully parenthesize binary exprs so that it works
      //in case onyx has a different operator precedence than C
      //all arithmetic operators have same behavior as C
      //(except + with array concatenation/prepend/append)
      if(binary->op == CMPEQ || binary->op == CMPNEQ ||
          binary->op == CMPL || binary->op == CMPLE ||
          binary->op == CMPG || binary->op == CMPGE)
      {
        c << ident << " = ";
        generateComparison(c, binary->op, binary->lhs->type, lhs, rhs);
      }
      else if(binary->op == PLUS &&
          binary->lhs->type->isArray() && binary->rhs->type->isArray())
      {
        //array concat
        c << getConcatFunc((ArrayType*) binary->lhs->type);
        c << "(" << ident << ", " << lhs << ", " << rhs << ");\n";
      }
      else if(binary->op == PLUS && binary->lhs->type->isArray())
      {
        //array append
        c << getAppendFunc((ArrayType*) binary->lhs->type);
        c << "(" << ident << ", " << lhs << ", " << rhs << ");\n";
      }
      else if(binary->op == PLUS && binary->rhs->type->isArray())
      {
        //array prepend
        c << getPrependFunc((ArrayType*) binary->rhs->type);
        c << "(" << ident << ", " << lhs << ", " << rhs << ");\n";
      }
      else
      {
        //primitive arithmetic
        c << ident << " = ";
        c << lhs << ' ' << operatorTable[binary->op] << ' ' << rhs << ");\n";
      }
    }
    else if(IntLiteral* intLit = dynamic_cast<IntLiteral*>(expr))
    {
      //allocate with the proper size and then assign
      c << ident << " = " << intLit->value;
      if(intLit->value > INT_MAX)
      {
        c << "LL";
      }
      c << ";\n";
    }
    else if(FloatLiteral* floatLit = dynamic_cast<FloatLiteral*>(expr))
    {
      char buf[80];
      //print enough digits to represent float exactly
      sprintf(buf, "%#.17f", floatLit->value);
      c << ident << " = " << buf << ";\n";
    }
    else if(StringLiteral* stringLit = dynamic_cast<StringLiteral*>(expr))
    {
      //allocate and populate char[] struct from string literal
      ArrayType* stringType = (ArrayType*) getArrayType(primitives[Prim::CHAR], 1);
      c << ident << " = " << getAllocFunc(stringType) << '(' << stringLit->value.length() << " + 1);\n";
      c << "memcpy(" << ident << "->data, \"";
      //output chars one at a time, escaping as needed
      for(auto ch : stringLit->value)
      {
        c << generateChar(ch);
      }
      c << "\", " << stringLit->value.length() + 1 << ");\n";
    }
    else if(CharLiteral* charLit = dynamic_cast<CharLiteral*>(expr))
    {
      c << ident << " = '";
      //generateChar produces escapes as needed
      c << generateChar(charLit->value);
      c << "';\n";
    }
    else if(BoolLiteral* boolLit = dynamic_cast<BoolLiteral*>(expr))
    {
      c << ident << " = ";
      c << (boolLit->value ? "true" : "false");
      c << ";\n";
    }
    else if(Indexed* indexed = dynamic_cast<Indexed*>(expr))
    {
      //Indexed expression must be either a tuple or array
      auto indexedType = indexed->group->type;
      string group = generateExpression(c, indexed->group);
      temporaries.emplace_back(indexedType, group);
      if(ArrayType* at = dynamic_cast<ArrayType*>(indexedType))
      {
        string index = generateExpression(c, indexed->index);
        temporaries.emplace_back(indexed->index->type, index);
        c << types[at->subtype] << ' ' << ident << " = ";
        string refOfElem = isPOD(at->type) ? "&" : "";
        c << getAccessFunc(at) << "(" << refOfElem << ident << ", " << group << ", ";
        c << index << ");\n";
      }
      else if(TupleType* tt = dynamic_cast<TupleType*>(indexedType))
      {
        int which = dynamic_cast<IntLiteral*>(indexed)->value;
        c << types[tt->members[which]] << ' ' << ident << " = ";
        c << group << ".mem" << which << ";\n";
      }
      else if(auto mt = dynamic_cast<MapType*>(indexedType))
      {
        //call hashFind and then put result in proper union type
        //first, figure out correct union tag values for Error and mt->value
        UnionType* ut = (UnionType*) expr->type;
        int errTag = -1;
        int valueTag = -1;
        for(size_t i = 0; i < ut->options.size(); i++)
        {
          if(ut->options[i] == primitives[Prim::ERROR])
            errTag = i;
          else if(ut->options[i] == mt->value)
            valueTag = i;
        }
        if(errTag == -1 || valueTag == -1)
        {
          INTERNAL_ERROR;
        }
        //the key, aka index
        string index = generateExpression(c, indexed->index);
        //form a union: (Value | Error)
        calc << types[ut] << ' ' << ident << ";\n";
        calc << "{\n";
        calc << "void* temp = hashFind(" << group << ", " << index << ");\n";
        calc << "if(temp)\n{\n";
        //data was found, so heap-allocate space for underlying data and copy to it
        calc << ident << ".data = malloc(sizeof(" << types[mt->value] << "));\n";
        calc << getCopyFunc(mt->value) << '(' << ident << ".data, temp);\n";
        calc << ident << ".tag = " << valueTag << ";\n";
        calc << "}\n";
        calc << "else\n{\n";
        //error is a unary type so the data field of union doesn't matter
        calc << ident << ".tag = " << errTag << ";\n";
        calc << "}\n";
      }
    }
    else if(CallExpr* call = dynamic_cast<CallExpr*>(expr))
    {
      if(auto subrExpr = dynamic_cast<SubroutineExpr*>(call->callable))
      {
        if(subrExpr->subr)
        {
          calc << types[call->type] << ' ' << ident << " = ";
          calc << subrs[subrExpr->subr] << '(';
          if(subrExpr->thisObject)
          {
            calc << '&' << generateExpression(c, subrExpr->thisObject);
          }
          for(size_t i = 0; i < call->args.size(); i++)
          {
            if(i > 0 || subrExpr->thisObject)
            {
              calc << ", ";
            }
            //this evaluates the arguments in order, before the call
            //non-primitives passed by ref
            if(!call->args[i]->type->isPrimitive())
            {
              calc << '&';
            }
            calc << generateExpression(c, call->args[i]);
          }
          calc << ");\n";
        }
        else
        {
          //external subroutine
          //just declare return value, generate argument values, and
          //output the C code, replacing $x with rv/args
          calc << types[call->type] << ' ' << ident << ";\n";
          vector<string> args;
          for(auto arg : call->args)
          {
            args.push_back(generateExpression(c, arg));
          }
          string& cCode = subrExpr->exSubr->c;
          auto subrType = subrExpr->exSubr->type;
          calc << "{\n";
          for(size_t i = 0; i < cCode.length();)
          {
            if(cCode[i] == '$')
            {
              i++;
              if(i == cCode.length() - 1)
              {
                errMsgLoc(subrExpr->exSubr, "$ is last character in external C code snippet");
              }
              char next = cCode[i];
              if(next == 'r')
              {
                //return value
                calc << ident;
              }
              else if(isdigit(next))
              {
                int argNum = 0;
                while(isdigit(cCode[i]))
                {
                  argNum *= 10;
                  argNum += cCode[i] - '0';
                  i++;
                }
                if(argNum < 0 || argNum >= subrType->argTypes.size())
                {
                  errMsgLoc(subrExpr->exSubr, "in C snippet, argument index "
                      << argNum << " out of bounds.\n");
                }
                calc << args[argNum];
              }
              else if(next == '$')
              {
                calc << '$';
              }
            }
            else
            {
              calc << cCode[i];
              i++;
            }
          }
          calc << ";}\n";
        }
      }
      else
      {
        calc << types[call->type] << ' ' << ident << " = ";
        //subroutine is a first-class function (any expression)
        //is a method call iff callable is a StructMem
        if(auto sm = dynamic_cast<StructMem*>(call->callable))
        {
          auto st = (StructType*) sm->base->type;
          //evaluate base only once (it is used twice)
          string base = generateExpression(c, sm->base);
          calc << '(' << base << ".mem";
          for(size_t i = 0; i < st->members.size(); i++)
          {
            if(st->members[i] == sm->member.get<Variable*>())
            {
              calc << i;
              break;
            }
          }
          calc << ")(&" << base;
          for(auto arg : call->args)
          {
            calc << ", ";
            if(!arg->type->isPrimitive())
            {
              calc << '&';
            }
            calc << generateExpression(c, arg);
          }
          calc << ");\n";
        }
        else
        {
          //just evaluate the expression which
          //is a function ptr, and do static call
          calc << generateExpression(c, call->callable) << '(';
          for(size_t i = 0; i < call->args.size(); i++)
          {
            if(i > 0)
            {
              calc << ", ";
            }
            if(!call->args[i]->type->isPrimitive())
            {
              calc << '&';
            }
            calc << generateExpression(c, call->args[i]);
          }
          calc << ");\n";
        }
      }
    }
    else if(NewArray* na = dynamic_cast<NewArray*>(expr))
    {
      calc << types[expr->type] << ' ' << ident << ";\n";
      //call the new array function with na's dimensions
      calc << getAllocFunc((ArrayType*) na->type);
      calc << "(&" << ident;
      for(int i = 0; i < na->dims.size(); i++)
      {
        calc << ", ";
        calc << generateExpression(c, na->dims[i]);
      }
      calc << ");\n";
    }
    else if(auto se = dynamic_cast<SubroutineExpr*>(expr))
    {
      calc << types[se->type] << ' ' << ident << " = " << subrs[se->subr] << ";\n";
    }
    else if(StructMem* sm = dynamic_cast<StructMem*>(expr))
    {
      calc << types[sm->type] << ' ' << ident << " = ";
      if(dynamic_cast<ThisExpr*>(sm->base->type))
      {
        calc << "(*this)";
      }
      else
      {
        calc << generateExpression(c, sm->base);
      }
      //find index of sm->member
      int memIndex = -1;
      StructType* st = (StructType*) sm->base->type;
      for(size_t i = 0; i < st->members.size(); i++)
      {
        if(sm->member.get<Variable*>() == st->members[i])
        {
          memIndex = i;
          break;
        }
      }
      calc << ".mem" << memIndex << ";\n";
    }
    else if(auto subExpr = dynamic_cast<SubroutineExpr*>(expr))
    {
      calc << types[expr->type] << ' ' << ident;
      calc << " = " << subrs[subExpr->subr] << ";\n";
    }
    else if(auto converted = dynamic_cast<Converted*>(expr))
    {
      calc << types[expr->type] << ' ' << ident << ";\n";
      getConvertFunc(converted->type, converted->value->type);
      calc << "(&" << ident << ", &";
      calc << generateExpression(c, converted->value) << ");\n";
    }
    else if(auto ee = dynamic_cast<EnumExpr*>(expr))
    {
      calc << types[expr->type] << ' ' << ident << " = " << ee->value << ";\n";
    }
    else if(auto cl = dynamic_cast<CompoundLiteral*>(expr))
    {
      //create a tuple representing compound lit,
      //and assign each member individually in a statement expression
      calc << types[expr->type] << ' ' << ident << ";\n";
      for(size_t i = 0; i < cl->members.size(); i++)
      {
        calc << getCopyFunc(cl->members[i]->type) << "(&" << ident << ".mem" << i;
        calc << ", " << generateExpression(c, cl->members[i]) << ");\n";
      }
    }
    else if(dynamic_cast<ErrorVal*>(expr))
    {
      calc << types[expr->type] << ' ' << ident << " = NULL;\n";
    }
    else if(ArrayLength* al = dynamic_cast<ArrayLength*>(expr))
    {
      calc << types[expr->type] << ' ' << ident << " = ";
      calc << generateExpression(c, al->array) << ".dim;\n";
    }
    else
    {
      //compound literal, or anything else that hasn't been covered
      INTERNAL_ERROR;
    }
    c << calc.str();
    return ident;
  }

  void generateComparison(ostream& c, int op, Type* t, string lhs, string rhs)
  {
    //comparison functions take pointers, so want to
    //take address of POD types, (non-POD types are already pointers)
    string refOf = isPOD(t) ? "&" : "";
    if(op == CMPEQ || op == CMPNEQ)
    {
      if(op == CMPNEQ)
      {
        c << '!';
      }
      c << getEqualsFunc(t) << "(" << refOf << lhs << ", " << refOf << rhs << ')';
    }
    else
    {
      //less = lhs < rhs
      //lesseq = !(rhs < lhs)
      //greater = rhs < lhs
      //greatereq = !(lhs < rhs)
      bool negate = (op == CMPLE || op == CMPGE);
      bool reverse = (op == CMPLE || op == CMPG);
      if(negate)
        c << '!';
      c << getLessFunc(t) << "(" << refOf << (reverse ? rhs : lhs) << ", " << refOf << (reverse ? lhs : rhs) << ')';
    }
  }

  void emitStatement(StatementIR* stmt)
  {
    if(auto ai = dynamic_cast<AssignIR*>(stmt))
    {
      //compute RHS
    }
    else if(auto call = dynamic_cast<CallIR*>(stmt))
    {
    }
    else if(auto j = dynamic_cast<Jump*>(stmt))
    {
    }
    else if(auto cj = dynamic_cast<CondJump*>(stmt))
    {
    }
    else if(auto label = dynamic_cast<Label*>(stmt))
    {
    }
    else if(auto ret = dynamic_cast<ReturnIR*>(stmt))
    {
    }
    else if(auto p = dynamic_cast<PrintIR*>(stmt))
    {
    }
    else if(auto assertion = dynamic_cast<AssertionIR*>(stmt))
    {
    }
    else
    {
      INTERNAL_ERROR;
    }
    return os;
  }


  void generateAssignment(ostream& c, Block* b, Expression* lhs, Expression* rhs)
  {
    //generateExpression can't be used with compound literals, so
    //  any case where LHS and/or RHS are compound literals are special
    //LHS is compound literal:
    //  -RHS can be another compound lit, or anything else is tuple, struct
    //LHS is variable or indexed:
    //  -RHS can be anything that matches type
    if(auto clLHS = dynamic_cast<CompoundLiteral*>(lhs))
    {
      //only compound literals, tuples and structs may be assigned to a compound literal
      if(auto clRHS = dynamic_cast<CompoundLiteral*>(rhs))
      {
        //copy members directly, one at a time
        for(size_t i = 0; i < clLHS->members.size(); i++)
        {
          generateAssignment(c, b, clLHS->members[i], clRHS->members[i]);
        }
      }
      else if(rhs->type->isTuple())
      {
        for(size_t i = 0; i < clLHS->members.size(); i++)
        {
          //create tuple index
          IntLiteral index(i);
          Indexed rhsMember(rhs, &index);
          generateAssignment(c, b, clLHS->members[i], &rhsMember);
        }
      }
      else if(rhs->type->isStruct())
      {
        //generate assignment for each member
        for(size_t i = 0; i < clLHS->members.size(); i++)
        {
          IntLiteral index(i);
          Indexed rhsMember(rhs, &index);
          generateAssignment(c, b, clLHS->members[i], &rhsMember);
        }
      }
    }
    else if(auto clRHS = dynamic_cast<CompoundLiteral*>(rhs))
    {
      //lhs may be a tuple, struct or array
      //know that lhs is not also a compound literal,
      //because that case is handled above
      if(lhs->type->isStruct())
      {
        //generate a StructMem expr for each member,
        //then generate the assignment to that
        //
        //semantic checking has already made sure that
        //compound lit members match 1-1 with struct members
        for(size_t i = 0; i < clRHS->members.size(); i++)
        {
          auto st = dynamic_cast<StructType*>(lhs->type);
          StructMem lhsMem(lhs, st->members[i]);
          generateAssignment(c, b, &lhsMem, clRHS->members[i]);
        }
      }
      else if(lhs->type->isTuple())
      {
        for(size_t i = 0; i < clRHS->members.size(); i++)
        {
          IntLiteral index(i);
          Indexed lhsMember(lhs, &index);
          generateAssignment(c, b, &index, clRHS->members[i]);
        }
      }
      else if(lhs->type->isArray())
      {
        //create the array with proper size,
        //then assign each element individually
        generateExpression(c, lhs);
        c << " = " << getAllocFunc((ArrayType*) lhs->type) << "(";
        c << clRHS->members.size() << ");\n";
        for(size_t i = 0; i < clRHS->members.size(); i++)
        {
          IntLiteral index(i);
          Indexed lhsMember(lhs, &index);
          generateAssignment(c, b, &lhsMember, clRHS->members[i]);
        }
      }
    }
    else if(auto indexed = dynamic_cast<Indexed*>(lhs))
    {
      //only arrays and maps can be base of 
      ArrayType* lhsArray = dynamic_cast<ArrayType*>(indexed->group->type);
      MapType* lhsMap = dynamic_cast<MapType*>(indexed->group->type);
      TupleType* lhsTuple = dynamic_cast<TupleType*>(indexed->group->type);
      if(lhsArray)
      {
        //call the "assign" func for array, which does bounds checking
        c << getAssignFunc(lhsArray) << '(';
        generateExpression(c, indexed->group);
        c << ", ";
        generateExpression(c, rhs);
        c << ", ";
        generateExpression(c, indexed->index);
        c << ");\n";
      }
      else if(lhsMap)
      {
        //use hash table insert
        c << "hashInsert(";
        generateExpression(c, indexed->group);
        c << ", ";
        generateExpression(c, indexed->index);
        c << ", ";
        generateExpression(c, rhs);
        c << ");\n";
      }
      else if(lhsTuple)
      {
        IntLiteral* index = (IntLiteral*)(indexed->index);
        generateExpression(c, indexed->group);
        c << "->mem " << index->value << " = ";
        generateExpression(c, rhs);
        c << ";\n";
      }
    }
    else
    {
      generateExpression(c, lhs);
      c << " = " << getCopyFunc(rhs->type) << "(";
      generateExpression(c, rhs);
      c << ");\n";
    }
  }

  void generateLocalVariables(ostream& c, Block* b)
  {
    for(auto& n : b->scope->names)
    {
      if(n.second.kind != Name::VARIABLE)
      {
        continue;
      }
      Variable* local = (Variable*) n.second.item;
      Type* type = local->type;
      string ident = getIdentifier();
      c << types[local->type] << ' ' << ident << ";\n";
      if(!type->isPrimitive())
      {
        string ptrIdent = getIdentifier();
        c << types[local->type] << "* " << ptrIdent << " = &" << ident << ";\n";
      }
      c << getInitFunc(local->type) << "(&" << ident << ");\n";
      vars[local] = ident;
    }
  }

  string getIdent(int i)
  {
    char name[16];
    sprintf(name, "O%x", i);
    return name;
  }

  string getIdentifier()
  {
    return getIdent(identCount++);
  }

  template<typename F>
    void walkScopeTree(F f)
    {
      vector<Scope*> visit;
      visit.push_back(global->scope);
      while(visit.size())
      {
        Scope* s = visit.back();
        f(s);
        visit.pop_back();
        for(auto child : s->children)
        {
          visit.push_back(child);
        }
      }
    }

  string getInitFunc(Type* t)
  {
    string& typeName = types[t];
    string func = "init_" + typeName + "_";
    //additional types where init funcs should be generated vector<Type*> deps;
    if(initImpl.find(t) != initImpl.end())
    {
      return func;
    }
    initImpl.insert(t);
    Oss def;
    utilFuncDecls << "void " << func << '(' << typeName << "* data);\n";
    def << "void " << func << '(' << typeName << "* data)\n{\n";
    if(t->isEnum())
    {
      EnumType* et = (EnumType*) t;
      if(et->values.size())
      {
        def << "*data = " << et->values[0]->value << ";\n";
      }
      else
      {
        //0
        def << "*data = 0;\n";
      }
    }
    else if(t->isStruct() || t->isTuple())
    {
      auto st = dynamic_cast<StructType*>(t);
      auto tt = dynamic_cast<TupleType*>(t);
      size_t numMembers = st ? st->members.size() : tt->members.size();
      for(size_t i = 0; i < numMembers; i++)
      {
        Type* memType = st ? st->members[i]->type : tt->members[i];
        def << getInitFunc(memType) << "(data->mem" << i << ");\n";
      }
    }
    else if(t->isArray())
    {
      def << "data->dim = 0;\n";
    }
    else if(t->isUnion())
    {
      UnionType* ut = (UnionType*) t;
      //union: just use the first tag and initialize it
      //since types in unions have to be sorted by pointer (arbitrary),
      //this isn't very useful but it does make union completely valid
      def << "data->tag = 0;\n";
      def << getInitFunc(ut->options[0]) << "(data->data);\n";
    }
    else if(t->isCallable())
    {
      def << "*data = nullptr;\n";
    }
    else if(t->isPrimitive())
    {
      //an empty array should have 0 size and NULL data
      //an uninitialized union should have tag 0 and NULL data
      //uninitialized fn ptr should be NULL for a clean segfault if called
      //in all these cases, the data is all 0 bytes
      def << "*data = 0;\n";
    }
    else if(auto mt = dynamic_cast<MapType*>(t))
    {
      //Uninitialized map type is all 0 bytes, except for hashFn and compareFn
      //note: getEqualsFunc produces bool(T*, T*) but really want bool(void*, void*)
      //so cast it here
      def << "typedef bool(*VoidComparator)(void*, void*);\n";
      def << "data->numBuckets = 0;\n";
      def << "data->size = 0;\n";
      def << "data->buckets = NULL;\n";
      def << "data->hashFn = " << getHashFunc(mt->key) << ";\n";
      def << "data->compareFn = (VoidComparator) " << getEqualsFunc(mt->key) << ";\n";
    }
    def << "}\n\n";
    utilFuncDefs << def.str();
    return func;
  }

  string getCopyFunc(Type* t)
  {
    string& typeName = types[t];
    string func = "copy_" + typeName + "_";
    if(copyImpl.find(t) != copyImpl.end())
    {
      return func;
    }
    copyImpl.insert(t);
    Oss def;
    {
      Oss prototype;
      prototype << "void " << func << '(' << typeName << "* dst, " << typeName << "* src)";
      utilFuncDecls << prototype.str() << ";\n";
      def << prototype.str() << "\n{\n";
    }
    //note: void doesn't get a copy function because it will never be called
    //cannot have variable or argument of type void (checked in middle end)
    if(t->isPrimitive())
    {
      //primitives (integers, bool, floats) are trivially copyable
      def << "*dst = *src;\n";
    }
    else if(auto at = dynamic_cast<ArrayType*>(t))
    {
      //allocate dst array
      def << "dst->dim = src->dim;\n";
      def << "dst->data = dst->dim * sizeof(" << typeName << ");\n";
      def << "for(" << size_type << " i = 0; i < src->dim; i++)\n{\n";
      def << getCopyFunc(at->subtype) << "(dst->data + i, src->data + i);\n";
      def << "}\n";
    }
    else if(t->isStruct() || t->isTuple())
    {
      auto st = dynamic_cast<StructType*>(t);
      auto tt = dynamic_cast<TupleType*>(t);
      size_t numMembers = st ? st->members.size() : tt->members.size();
      for(size_t i = 0; i < numMembers; i++)
      {
        Type* memType = st ? st->members[i]->type : tt->members[i];
        def << getCopyFunc(memType) << "(&(dst->mem" << i << "), &(src->mem" << i << "));\n";
      }
    }
    else if(auto ut = dynamic_cast<UnionType*>(t))
    {
      def << "dst->tag = src->tag;\n";
      def << "switch(dst->tag)\n{\n";
      for(size_t i = 0; i < ut->options.size(); i++)
      {
        Type* opType = ut->options[i];
        def << "case " << i << ":\n";
        getCopyFunc(opType);
        def << "((" << types[opType] << "*) dst->data, (" << types[opType] << "*) src->data);\n";
        def << "break;\n";
      }
      def << "default: panic(\"union has invalid tag which should not be possible\");\n}\n";
    }
    else if(auto mt = dynamic_cast<MapType*>(t))
    {
      //clone the hash table: same # of buckets, same bucket capacities
      def << "dst->hashFn = src->hashFn;\n";
      def << "dst->compareFn = src->compareFn;\n";
      def << "dst->size = src->size;\n";
      def << "dst->numBuckets = src->numBuckets;\n";
      def << "for(" << size_type << " i = 0; i < dst->numBuckets; i++)\n{\n";
      def << "dst->buckets[i]->cap = src->buckets[i]->cap;\n";
      def << "dst->buckets[i]->size = src->buckets[i]->size;\n";
      def << "dst->buckets[i]->keys = malloc(sizeof(void*) * src->buckets[i]->cap;\n";
      def << "dst->buckets[i]->values = malloc(sizeof(void*) * src->buckets[i]->cap;\n";
      def << "dst->buckets[i]->hashes = malloc(sizeof(uint32_t) * src->buckets[i]->cap;\n";
      def << "for(int j = 0; j < dst->buckets[i]->size; j++)\n{\n";
      //have to heap allocate each dst key and value individually
      def << "dst->buckets[i]->keys[j] = malloc(sizeof(" << types[mt->key] << "));\n";
      def << getCopyFunc(mt->key) << "((" << types[mt->key];
      def << "*) dst->buckets[i]->keys[j], (" << types[mt->key];
      def << "*) src->buckets[i]->keys[j]);\n";
      def << "dst->buckets[i]->values[j] = malloc(sizeof(" << types[mt->value] << "));\n";
      def << getCopyFunc(mt->value) << "((" << types[mt->value];
      def << "*) dst->buckets[i]->values[j], (" << types[mt->value];
      def << "*) src->buckets[i]->values[j]);\n";
      def << "dst->buckets[i]->hashes[j] = src->buckets[i]->hashes[j];\n";
      def << "}\n";
      def << "}\n";
    }
    def << "return cp;\n}\n\n";
    utilFuncDefs << def.str();
    return func;
  }

  string getAllocFunc(ArrayType* t)
  {
    string func = "alloc_" + types[t] + "_";
    string& typeName = types[t];
    if(allocImpl.find(t) != allocImpl.end())
    {
      return func;
    }
    allocImpl.insert(t);
    //names of arguments
    vector<string> args;
    for(int i = 0; i < t->dims; i++)
    {
      args.push_back(getIdentifier());
    }
    Oss def;
    {
      Oss prototype;
      prototype << "void " << func << '(';
      prototype << typeName << "* data";
      for(int i = 0; i < t->dims; i++)
      {
        prototype << ", ";
        prototype << size_type << ' ' << args[i];
      }
      prototype << ')';
      utilFuncDecls << prototype.str() << ";\n";
      def << prototype.str() << "\n{\n";
    }
    //add prototype to both util decls and defs
    Type* subtype = t->subtype;
    //allocate top level array
    def << "data->dim = " << args[0] << ";\n";
    def << "data->data = malloc(data->dim * sizeof(" << types[subtype] << "));\n";
    def << "for(" << size_type << " i = 0; i < " << args[0] << "; i++)\n{\n";
    if(auto subArray = dynamic_cast<ArrayType*>(subtype))
    {
      //call allocate function for subtype
      def << getAllocFunc(subArray) << "(data->data + i";
      //pass in all args to this function, except first one
      for(int i = 1; i < args.size(); i++)
      {
        def << ", " << args[i];
      }
      def << ");\n";
    }
    else
    {
      //initialize innermost data, which is not an array
      def << getInitFunc(subtype) << "(data->data + i);\n";
    }
    def << "}\n}\n\n";
    utilFuncDefs << def.str();
    return func;
  }

  string getDeallocFunc(Type* t)
  {
    string func = "free_" + types[t] + "_";
    if(deallocImpl.find(t) != deallocImpl.end())
    {
      return func;
    }
    deallocImpl.insert(t);
    string& typeName = types[t];
    //only struct, tuple, array and unions need to be freed
    utilFuncDecls << "void " << func << "(" << typeName << "* data);\n";
    Oss def;
    def << "void " << func << "(" << typeName << " data)\n{\n";
    //special case: this is actually a no-op, but need this empty function to exist for whatever reason
    if(!typeNeedsDealloc(t))
    {
      def << "}\n\n";
      utilFuncDefs << def.str();
      return func;
    }
    if(auto st = dynamic_cast<StructType*>(t))
    {
      for(size_t i = 0; i < st->members.size(); i++)
      {
        if(typeNeedsDealloc(st->members[i]->type))
        {
          def << getDeallocFunc(st->members[i]->type) << "(&data->mem" << i << ");\n";
        }
      }
    }
    else if(auto tt = dynamic_cast<TupleType*>(t))
    {
      for(size_t i = 0; i < tt->members.size(); i++)
      {
        if(typeNeedsDealloc(tt->members[i]))
        {
          def << getDeallocFunc(tt->members[i]) << "(&data->mem" << i << ");\n";
        }
      }
    }
    else if(auto ut = dynamic_cast<UnionType*>(t))
    {
      def << "switch(data->tag)\n{\n";
      for(size_t i = 0; i < ut->options.size(); i++)
      {
        def << "case " << i << ":\n";
        //union data is void* which can be casted directly to actual type
        def << getDeallocFunc(ut->options[i]) << "((" << types[ut->options[i]] << "*) data->data);\n";
        def << "break;\n";
      }
      def << "default: panic(\"union being deallocated has invalid tag\");\n";
      def << "}\n";
    }
    else if(auto at = dynamic_cast<ArrayType*>(t))
    {
      //add free calls for each element, if subtype has nontrivial deallocator
      if(typeNeedsDealloc(at->subtype))
      {
        def << "for(" << size_type << " i = 0; i < data->dim; i++)\n{\n";
        def << getDeallocFunc(at->subtype) << "(data->data + i);\n";
        def << "}\n";
      }
      def << "free(data->data);\n";
    }
    else if(auto mt = dynamic_cast<MapType*>(t))
    {
      //have a hash table: free all the key/value pairs, then each
      //bucket's arrays, then the array of buckets
      def << "for(" << size_type << " b = 0; b < data->numBuckets; b++)\n";
      def << "{\n";
      def << "for(" << size_type << " i = 0; i < data->buckets[b]->size; i++)\n";
      def << "{\n";
      def << getDeallocFunc(mt->key) << "((" << types[mt->key] << "*) data->buckets[b]->keys[i]);\n";
      def << "free(data->buckets[b]->keys[i]);\n";
      def << getDeallocFunc(mt->value) << "((" << types[mt->value] << "*) data->buckets[b]->values[i]);\n";
      def << "free(data->buckets[b]->values[i]);\n";
      def << "}\n";
      def << "free(buckets[b]->keys);\n";
      def << "free(buckets[b]->values);\n";
      def << "free(buckets[b]->hashes);\n";
      def << "}\n";
      def << "free(data->buckets);\n";
    }
    def << "}\n";
    utilFuncDefs << def.str();
    return func;
  }

  string getPrintFunc(Type* t)
  {
    string& typeName = types[t];
    string func = "print_" + typeName + "_";
    if(printImpl.find(t) != printImpl.end())
    {
      return func;
    }
    printImpl.insert(t);
    Oss def;
    Oss prototype;
    prototype << "void " << func << "(" << typeName << "* data)";
    utilFuncDecls << prototype.str() << ";\n";
    def << prototype.str() << "\n{\n";
    if(t->isPrimitive())
    {
      //all primitives except bool can be printed as a single printf specifier
      //so just determine the %format
      string fmt;
      if(auto intType = dynamic_cast<IntegerType*>(t))
      {
        //printf format code
        switch(intType->size)
        {
          case 1:
            fmt = intType->isSigned ? "hhd" : "hhu";
            break;
          case 2:
            fmt = intType->isSigned ? "hd" : "hu";
            break;
          case 4:
            fmt = intType->isSigned ? "d" : "u";
            break;
          case 8:
            fmt = intType->isSigned ? "lld" : "llu";
            break;
          default:
            INTERNAL_ERROR;
        }
      }
      else if(t->isChar())
      {
        fmt = "c";
      }
      else if(t->isFloat())
      {
        fmt = "f";
      }
      def << "printf(\"%" << fmt << "\", *data);\n";
    }
    else if(t->isBool())
    {
      def << "if(*data)\nprintf(\"true\");\nelse\nprintf(\"false\");\n";
    }
    else
    {
      //compound types
      if(ArrayType* at = dynamic_cast<ArrayType*>(t))
      {
        if(at->subtype->isChar())
        {
          //t is string (special case): print the chars verbatim
          def << "for(" << size_type << " i = 0; i < data->dim; i++)\n{\n";
          def << "putchar(*(data->data[i]));\n";
          def << "}\n";
        }
        else
        {
          def << "putchar('{');\n";
          def << "for(" << size_type << " i = 0; i < data->dim; i++)\n{\n";
          def << getPrintFunc(at->subtype) << "(data->data + i);\n}\n";
          def << "putchar('}');\n";
        }
      }
      else if(TupleType* tt = dynamic_cast<TupleType*>(t))
      {
        def << "putchar('(');\n";
        //print each member, comma separated
        for(size_t i = 0; i < tt->members.size(); i++)
        {
          def << getPrintFunc(tt->members[i]) << "(&data->mem" << i << ");\n";
          if(i != tt->members.size() - 1)
          {
            def << "printf(\", \");\n";
          }
        }
        def << "putchar(')');\n";
      }
      else if(StructType* st = dynamic_cast<StructType*>(t))
      {
        def << "printf(\"" << st->getName() << "{\");\n";
        //print each member, comma separated
        for(size_t i = 0; i < st->members.size(); i++)
        {
          def << getPrintFunc(st->members[i]->type) << "(&data->mem" << i << ");\n";
          if(i != st->members.size() - 1)
          {
            def << "printf(\", \");\n";
          }
        }
        def << "putchar('}');\n";
      }
      else if(UnionType* ut = dynamic_cast<UnionType*>(t))
      {
        def << "printf(\"" << ut->getName() << ':';
        def << "switch(data->tag)\n{\n";
        for(size_t i = 0; i < ut->options.size(); i++)
        {
          def << "case " << i << ":\n";
          def << getPrintFunc(ut->options[i]) << "((" << types[ut->options[i]] << "*) data->data));\n";
          def << "break;\n";
        }
        def << "default:;\n";
        def << "}\n";
      }
      else if(MapType* mt = dynamic_cast<MapType*>(t))
      {
        //format: {key: value, key: value, ...}
        def << "putchar('{');\n";
        def << "for(size_t i = 0; i < data->numBuckets; i++)\n{\n";
        def << "for(size_t j = 0; j < data->buckets[i]->size; j++)\n{\n";
        def << getPrintFunc(mt->key) << "(data->buckets[i]->keys[j]);\n";
        def << "printf(\": \");\n";
        def << getPrintFunc(mt->value) << "(data->buckets[i]->values[j]);\n";
        def << "if(i == data->numBuckets - 1 && j == data->buckets[i]->size - 1)\n{\n";
        def << "printf(\", \");\n";
        def << "}\n}\n}\n";
        def << "putchar('}');\n";
      }
    }
    def << "}\n\n";
    utilFuncDefs << def.str();
    return func;
  }

  string getConvertFunc(Type* out, Type* in)
  {
    string func = "convert_" + types[out] + "_" + types[in] + "_";
    pair<Type*, Type*> typePair(out, in);
    if(convertImpl.find(typePair) != convertImpl.end())
    {
      return func;
    }
    convertImpl.insert(typePair);
    Oss def;
    {
      Oss prototype;
      prototype << "void " << func << '(' << types[out] << "* dst, " << types[in] << "* src)";
      utilFuncDecls << prototype.str() << ";\n";
      def << prototype.str() << "\n{\n";
    }
    //All supported type conversions:
    //  (case 1) -All primitives can be converted to each other trivially
    //    -floats/doubles truncated to integer as in C
    //    -ints converted to each other as in C
    //    -char treated as integer
    //    -any number converted to bool with nonzero being true
    //  (case 2) -Out = struct: in = struct or tuple
    //  (case 3) -Out = array: in = struct, tuple or array
    //  (case 4) -Out = map: in = map, array, or tuple
    //    -in = map: convert keys to keys and values to values;
    //      since maps are unordered, key conflicts are UB
    //    -in = array/tuple: key is int, values converted to values
    //    -in = struct: key is string, value 
    //  (case 2) -Out = tuple: in = struct or tuple
    //conversion from one primitive to another is same semantics as C
    if(in->isPrimitive() && out->isPrimitive())
    {
      if(out == primitives[Prim::BOOL])
      {
        //like C, out is true if and only if in is nonzero
        def << "*dst = *src != 0;\n";
      }
      else
      {
        //primitive: explicitly cast *in to out type
        def << "*dst = (" << types[out] << ") *src;\n";
      }
    }
    else if((out->isStruct() || out->isTuple()) &&
        (in->isStruct() || in->isTuple()))
    {
      StructType* inStruct = dynamic_cast<StructType*>(in);
      StructType* outStruct = dynamic_cast<StructType*>(out);
      TupleType* inTuple = dynamic_cast<TupleType*>(in);
      TupleType* outTuple = dynamic_cast<TupleType*>(out);
      def << types[out] << "* out = malloc(sizeof(" << types[out] << "));\n";
      //deep copy members, one at a time
      //do explicit conversion where necessary
      size_t n = 0;
      if(outStruct)
        n = outStruct->members.size();
      else
        n = outTuple->members.size();
      for(size_t i = 0; i < n; i++)
      {
        Type* inMem;
        Type* outMem;
        if(inStruct)
          inMem = inStruct->members[i]->type;
        else
          inMem = inTuple->members[i];
        if(outStruct)
          outMem = outStruct->members[i]->type;
        else
          outMem = outTuple->members[i];
        if(inMem == outMem)
        {
          def << "out->mem" << i << " = " << getCopyFunc(inMem) << '(' <<
            "in->mem" << i << ");\n";
        }
        else
        {
          def << "out->mem" << i << " = " << getConvertFunc(outMem, inMem) <<
            '(' << "in->mem" << i << ");\n";
        }
      }
      def << "return out;\n";
    }
    else if(out->isArray())
    {
      ArrayType* outArray = (ArrayType*) out;
      //1 of these 3 pointers will be non-null
      StructType* inStruct = dynamic_cast<StructType*>(in);
      TupleType* inTuple = dynamic_cast<TupleType*>(in);
      ArrayType* inArray = dynamic_cast<ArrayType*>(in);
      //first, allocate correctly sized array
      def << types[out] << "* out = " << getAllocFunc(outArray) << '(';
      if(inStruct)
      {
        def << inStruct->members.size();
      }
      else if(inTuple)
      {
        def << inTuple->members.size();
      }
      else if(inArray)
      {
        def << "in->dim";
      }
      else
      {
        //only array, struct and tuple may be converted to array
        //if here, semantic checker has missed something
        INTERNAL_ERROR;
      }
      def << ");\n";
      //copy members, converting as necessary
      Type* outSubtype = outArray->subtype;
      if(inStruct || inTuple)
      {
        size_t n = inStruct ? inStruct->members.size() : inTuple->members.size();
        for(size_t i = 0; i < n; i++)
        {
          Type* inMem = nullptr;
          if(inStruct)
            inMem = inStruct->members[i]->type;
          else
            inMem = inTuple->members[i];
          def << "out->data[" << i << " = ";
          if(outSubtype == inMem)
          {
            def << getCopyFunc(outSubtype);
          }
          else
          {
            def << getConvertFunc(outSubtype, inMem);
          }
          def << "(in->mem" << i << ");\n";
        }
      }
      else
      {
        Type* inSubtype = inArray->subtype;
        //array: generate a C loop that copies or converts members
        def << "for(" << size_type << " i = 0; i < in->dim; i++)\n{\n";
        def << "out->data[i] = ";
        if(outSubtype == inSubtype)
          def << getCopyFunc(outSubtype);
        else
          def << getConvertFunc(outSubtype, inSubtype);
        def << "(in->data[i]);\n";
        def << "}\n";
      }
      def << "return out;\n";
    }
    else if(out->isUnion())
    {
      UnionType* ut = (UnionType*) out;
      int option = -1;
      //check for an exactly matching type first
      for(size_t i = 0; i < ut->options.size(); i++)
      {
        if(ut->options[i] == in)
        {
          option = i;
          break;
        }
      }
      //no type matches exactly: use first option that in may convert to
      if(option == -1)
      {
        for(size_t i = 0; i < ut->options.size(); i++)
        {
          if(ut->options[i]->canConvert(in))
          {
            option = i;
            break;
          }
        }
      }
      if(option ==  -1)
      {
        INTERNAL_ERROR;
      }
      Type* opType = ut->options[option];
      def << types[out] << "* temp = malloc(sizeof(" << types[out] << "));\n";
      if(opType == in)
      {
        def << "temp->data = " << getCopyFunc(in) << "(in);\n";
      }
      else
      {
        def << "temp->data = " << getConvertFunc(opType, in) << "(in);\n";
      }
      def << "temp->tag = " << option << ";\n";
      //now return union with correct tag and temp_ as the data
      def << "return temp;\n";
    }
    else if(out->isMap())
    {
      MapType* outMap = (MapType*) out;
      //in is either an array of (key, value) tuples, itself a map
      //In either case, just iterate over the k-v pairs and insert them
      MapType* inMap = dynamic_cast<MapType*>(in);
      ArrayType* inArray = dynamic_cast<ArrayType*>(in);
      //allocate and initialize hash table
      def << types[out] << "* temp = calloc(1, sizeof(" << types[out] << "));\n";
      def << "temp->hashFn = " << getHashFunc(outMap->key) << ";\n";
      def << "temp->compareFn = " << getEqualsFunc(outMap->key) << ";\n";
      if(inMap)
      {
        //"void hashInsert(HashTable* table, void* key, void* data)\n{\n"
        def << "for(int i = 0; i < in->numBuckets; i++)\n{\n";
        def << "Bucket* b = in->buckets + i;\n";
        def << "for(int j = 0; j < b->size; j++)\n{\n";
        def << "hashInsert(temp, ";
        //convert key to outMap->key if necessary
        if(inMap->key != outMap->key)
        {
          def << getConvertFunc(outMap->key, inMap->key);
          def << "(b->keys[j]), ";
        }
        else
        {
          def << "b->keys[j], ";
        }
        //same with value
        if(inMap->value != outMap->value)
        {
          def << getConvertFunc(outMap->value, inMap->value);
          def << "(b->values[j]));\n";
        }
        else
        {
          def << "b->values[j]);\n";
        }
        def << "}\n}\n";
      }
      else if(inArray)
      {
        TupleType* kv = dynamic_cast<TupleType*>(inArray->subtype);
        if(!kv || kv->members.size() != 2)
        {
          INTERNAL_ERROR;
        }
        Type* inKey = kv->members[0];
        Type* inValue = kv->members[1];
        def << "for(int i = 0; i << in->dim; i++)\n{\n";
        def << "hashInsert(temp, ";
        if(inKey != outMap->key)
        {
          def << getConvertFunc(outMap->key, inKey) << "(in->data[i]->mem0), ";
        }
        else
        {
          def << "in->data[i], ";
        }
        if(inValue != outMap->value)
        {
          def << getConvertFunc(outMap->value, inValue) << "(in->data[i]->mem1), ";
        }
        else
        {
          def << "in->data[i]->mem1);\n";
        }
        def << "}\n";
      }
      else
      {
        INTERNAL_ERROR;
      }
    }
    def << "}\n\n";
    utilFuncDefs << def.str();
    return func;
  }

  string getEqualsFunc(Type* t)
  {
    string& typeName = types[t];
    string func = "equals_" + typeName + "_";
    if(equalsImpl.find(t) != equalsImpl.end())
    {
      return func;
    }
    equalsImpl.insert(t);
    Oss def;
    Oss prototype;
    prototype << "bool " << func << '(' << typeName << "* lhs, " << typeName << "* rhs)";
    utilFuncDecls << prototype.str() << ";\n";
    def << prototype.str() << "\n{\n";
    StructType* st = dynamic_cast<StructType*>(t);
    TupleType* tt = dynamic_cast<TupleType*>(t);
    if(t->isPrimitive() || t->isCallable())
    {
      def << "return *lhs == *rhs;\n";
    }
    else if(st || tt)
    {
      int nMem = st ? st->members.size() : tt->members.size();
      for(int i = 0; i < nMem; i++)
      {
        Type* mem = st ? st->members[i]->type : tt->members[i];
        def << "if(!" << getEqualsFunc(mem) << "(lhs->mem" << i <<
          ", rhs->mem" << i << "))\nreturn false;\n";
      }
      def << "return true;\n";
    }
    else if(ArrayType* at = dynamic_cast<ArrayType*>(t))
    {
      def << "if(lhs->dim != rhs->dim)\nreturn false;\n";
      def << "for(" << size_type << " i = 0; i < lhs->dim; i++)\n";
      def << "{\nif(!" << getEqualsFunc(at->subtype) << "(lhs->data[i], rhs->data[i]))\n";
      def << "return false;}\n";
      def << "return true;\n";
    }
    else if(UnionType* ut = dynamic_cast<UnionType*>(t))
    {
      //lhs and rhs must be exactly the same union type,
      //so compare tags and then data
      def << "if(lhs->tag != rhs->tag)\n{\nreturn false;\n}\n";
      //underlying data of lhs and rhs have same type; compare them
      def << "switch(lhs->tag)\n{\n";
      for(size_t i = 0; i < ut->options.size(); i++)
      {
        def << "case " << i << ":\n";
        def << "return " << getEqualsFunc(ut->options[i]) << "((" <<
          types[ut->options[i]] << "*) lhs->data), (" << types[ut->options[i]] << "*) rhs->data);\n";
      }
      def << "default: return false;\n}\n";
      def << "return false;\n";
    }
    else if(MapType* mt = dynamic_cast<MapType*>(t))
    {
      def << "if(lhs->size != rhs->size)\nreturn false;\n";
      //Hash tables can be equal but still have different #s of buckets,
      //or have items in different order within buckets
      def << "for(int i = 0; i < lhs->numBuckets; i++)\n{\n";
      def << "Bucket* b = lhs->buckets + j;\n";
      def << "for(int j = 0; j < b->size; j++)\n{\n";
      //given a k-v pair in lhs, search for k in rhs
      def << "void* found = hashFind(rhs, b->keys[j]);\n";
      def << "if(!found || !" << getEqualsFunc(mt->value) << "(b->values[j], (" << types[mt->value] << "*) found))\n";
      def << "return false;\n";
      def << "}\n";
      def << "return true;\n}\n";
    }
    else
    {
      INTERNAL_ERROR;
    }
    def << "}\n";
    utilFuncDefs << def.str();
    return func;
  }

  string getLessFunc(Type* t)
  {
    string& typeName = types[t];
    string func = "less_" + typeName + "_";
    if(lessImpl.find(t) != lessImpl.end())
    {
      return func;
    }
    lessImpl.insert(t);
    Oss def;
    Oss prototype;
    prototype << "bool " << func << '(' << typeName << "* lhs, " << typeName << "* rhs)";
    utilFuncDecls << prototype.str() << ';';
    def << prototype.str() << "\n{\n";
    if(t->isPrimitive() || t->isCallable())
    {
      if(t->isBool())
        def << "return !(*lhs) && *rhs;\n";
      else
        def << "return *lhs < *rhs;\n";
    }
    else if(StructType* st = dynamic_cast<StructType*>(t))
    {
      for(size_t i = 0; i < st->members.size(); i++)
      {
        Type* mem = st->members[i]->type;
        def << "if(" << getLessFunc(mem) << "(lhs->mem" << i <<
          ", rhs->mem" << i << "))\nreturn true;\n";
        def << "else if(" << getLessFunc(mem) << "(rhs->mem" << i <<
          ", rhs->mem" << i << "))\nreturn false;\n";
      }
      def << "return false;\n";
    }
    else if(TupleType* tt = dynamic_cast<TupleType*>(t))
    {
      for(size_t i = 0; i < tt->members.size(); i++)
      {
        def << "if(" << getLessFunc(tt->members[i]) << "(lhs->mem" << i <<
          ", rhs->mem" << i << "))\nreturn true;\n";
        def << "else if(" << getLessFunc(tt->members[i]) << "(rhs->mem" << i <<
          ", lhs->mem" << i << "))\nreturn false;\n";
      }
      def << "return false;\n";
    }
    else if(ArrayType* at = dynamic_cast<ArrayType*>(t))
    {
      def << "if(lhs->dim < rhs->dim)\nreturn true;\n";
      def << "else if(lhs->dim > rhs->dim)\nreturn false;\n";
      def << "for(" << size_type << " i = 0; i < lhs->dim; i++)\n";
      def << "{\n";
      def << "if(" << getLessFunc(at->subtype) << "(lhs->data[i], rhs->data[i]))\n";
      def << "return true;";
      def << "else if(" << getLessFunc(at->subtype) << "(rhs->data[i], lhs->data[i]))\n";
      def << "return false;";
      def << "}\n";
      def << "return false;\n";
    }
    else if(UnionType* ut = dynamic_cast<UnionType*>(t))
    {
      //lhs and rhs must be exactly the same union type,
      //so compare tags and then data
      def << "if(lhs->tag < rhs->tag)\nreturn true;\n";
      def << "else if(lhs->tag > rhs->tag)\nreturn true;\n";
      //need to compare underlying data for the actual type
      def << "switch(lhs->tag)\n{\n";
      for(size_t i = 0; i < ut->options.size(); i++)
      {
        def << "case " << i << ":\n";
        def << "return " << getLessFunc(ut->options[i]) << "((" <<
          types[ut->options[i]] << "*) lhs->data), (" << types[ut->options[i]] << "*) rhs->data));\n";
      }
      def << "default: return false;\n}\n";
      def << "return false;\n";
    }
    else if(MapType* mt = dynamic_cast<MapType*>(t))
    {
      //must take the keys from both maps, sort them, and then
      def << "if(lhs->size < rhs->size)\nreturn true;\n";
      def << "if(lhs->size > rhs->size)\nreturn false;\n";
      //lex compare keys then values (if key arrays identical)
      Type* key = mt->key;
      Type* val = mt->value;
      //note: since sorted copies of keys are only live in this function,
      //can safely shallow copy them
      def << types[key] << "** lhsKeys = malloc(lhs->size * sizeof(void*));\n";
      //it will count how many keys have been inserted so far
      def << size_type << " it = 0;\n";
      def << "for(" << size_type << " i = 0; i < lhs->numBuckets; i++)\n{\n";
      def << "for(" << size_type << " j = 0; J < lhs->buckets[i]->size; j++)\n{\n";
      def << "lhsKeys[it] = lhs->buckets[i]->keys[j];\n";
      def << "it++;\n";
      def << "}\n";
      def << "}\n";
      def << types[key] << "** rhsKeys = malloc(rhs->size * sizeof(void*));\n";
      def << size_type << " it = 0;\n";
      def << "for(" << size_type << " i = 0; i < rhs->numBuckets; i++)\n{\n";
      def << "for(" << size_type << " j = 0; J < rhs->buckets[i]->size; j++)\n{\n";
      def << "rhsKeys[it] = rhs->buckets[i]->keys[j];\n";
      def << "it++;\n";
      def << "}\n";
      def << "}\n";
      //sort lhsKeys and rhsKeys according to getLessFunc(key)
      def << getSortFunc(key) << "(lhsKeys, lhs->size);\n";
      def << getSortFunc(key) << "(rhsKeys, rhs->size);\n";
      //lex compare the keys
      def << "for(" << size_type << " i = 0; i < lhs->size; i++)\n{\n";
      def << "if(" << getLessFunc(key) << "(lhsKeys[i], rhsKeys[i]))\n";
      def << "return true;\n";
      def << "if(!" << getEqualsFunc(key) << "(lhsKeys[i], rhsKeys[i]))\n";
      def << "return false;\n";
      def << "}\n";
      //lhs and rhs have exactly the same set of keys, so compare the values
      def << "for(" << size_type << " i = 0; i < lhs->size; i++)\n{\n";
      def << types[val] << "* lhsValue = (" << types[val] << "*) hashFind(lhs, lhsKeys[i]);\n";
      def << types[val] << "* rhsValue = (" << types[val] << "*) hashFind(rhs, lhsKeys[i]);\n";
      def << "if(" << getLessFunc(val) << "(lhsValue, rhsValue))\n";
      def << "return true;\n";
      def << "if(!" << getEqualsFunc(val) << "(lhsValue, rhsValue))\n";
      def << "return false;\n";
      def << "}\n";
      def << "return true;\n";
    }
    else
    {
      INTERNAL_ERROR;
    }
    def << "}\n\n";
    utilFuncDefs << def.str();
    return func;
  }

  string getConcatFunc(ArrayType* at)
  {
    string& typeName = types[at];
    string func = "concat_" + typeName + '_';
    if(concatImpl.find(at) != concatImpl.end())
    {
      return func;
    }
    concatImpl.insert(at);
    Oss prototype;
    prototype << typeName << "* " << func << '(' << typeName;
    prototype << "* lhs, " << typeName << "* rhs)";
    utilFuncDecls << prototype.str() << ";\n";
    Oss def;
    def << prototype.str() << "\n{\n";
    Type* subtype = at->subtype;
    def << types[subtype] << "** newData = " <<
      "malloc((lhs->dim + rhs->dim) * sizeof(void*));\n";
    def << "for(" << size_type << " i = 0; i < lhs->dim; i++)\n{\n";
    def << "newData[i] = " << getCopyFunc(subtype) << "(lhs->data[i]);\n";
    def << "}\n";
    def << "for(" << size_type << " i = 0; i < rhs->dim; i++)\n{\n";
    def << "newData[i + lhs->dim] = " << getCopyFunc(subtype) <<
      "(rhs->data[i]);\n";
    def << "}\n";
    def << typeName << " rv = malloc(sizeof(" << typeName << "));\n";
    def << "rv->data = newData;\n";
    def << "rv->dim = lhs->dim + rhs->dim;\n";
    def << "return rv;\n}\n\n";
    utilFuncDefs << def.str();
    return func;
  }

  string getAppendFunc(ArrayType* at)
  {
    string& typeName = types[at];
    string func = "append_" + typeName + '_';
    if(appendImpl.find(at) != appendImpl.end())
    {
      return func;
    }
    appendImpl.insert(at);
    Oss prototype;
    Type* subtype = at->subtype;
    prototype << typeName << "* " << func << '(' << typeName;
    prototype << "* lhs, " << types[subtype] << "* rhs)";
    utilFuncDecls << prototype.str() << ";\n";
    Oss def;
    def << prototype.str() << "\n{\n";
    //allocate new array "rv_"
    def << types[subtype] << "** newData = " <<
      "malloc(sizeof(void*) * (lhs->dim + 1));\n";
    def << "for(" << size_type << "i = 0; i < lhs->dim; i++)\n{\n";
    def << "newData[i] = " << getCopyFunc(subtype) << "(lhs->data[i]);\n";
    def << "}\n";
    def << "newData[lhs->dim] = " << getCopyFunc(subtype) << "(rhs);\n";
    def << typeName << "* rv = malloc(sizeof(" << typeName << "));\n";
    def << "rv->data = newData;\n";
    def << "rv->dim = lhs->dim + 1;\n";
    def << "return rv;\n}\n\n";
    utilFuncDefs << def.str();
    return func;
  }

  string getPrependFunc(ArrayType* at)
  {
    string& typeName = types[at];
    string func = "prepend_" + typeName + '_';
    if(prependImpl.find(at) != prependImpl.end())
    {
      return func;
    }
    prependImpl.insert(at);
    Oss prototype;
    Type* subtype = at->subtype;
    prototype << typeName << "* " << func << '(' << types[subtype];
    prototype << "* lhs, " << typeName << "* rhs)";
    utilFuncDecls << prototype.str() << ";\n";
    Oss def;
    def << prototype.str() << "\n{\n";
    //allocate new array "rv_"
    def << types[subtype] << "** newData = " <<
      "malloc(sizeof(void*) * (rhs->dim + 1));\n";
    def << "newData[0] = " << getCopyFunc(subtype) << "(lhs);\n";
    def << "for(" << size_type << "i = 0; i < rhs->dim; i++)\n{\n";
    def << "newData[1 + i] = " << getCopyFunc(subtype) << "(rhs->data[i]);\n";
    def << "}\n";
    def << typeName << "* rv = malloc(sizeof(" << typeName << "));\n";
    def << "rv->data = newData;\n";
    def << "rv->dim = rhs->dim + 1;\n";
    def << "return rv;\n}\n\n";
    utilFuncDefs << def.str();
    return func;
  }

  string getSortFunc(Type* t)
  {
    string& typeName = types[t];
    string func = "sort_" + typeName + '_';
    if(sortImpl.find(t) != sortImpl.end())
    {
      return func;
    }
    sortImpl.insert(t);
    Oss prototype;
    prototype << "void " << func << '(' << typeName;
    prototype << "** data, " << size_type << " n)";
    utilFuncDecls << prototype.str() << ";\n";
    //Generate a C stdlib qsort comparator, returns:
    //-1 for l < r
    //0 for l == r
    //1 for l > r
    //qsortComp doesn't need a separate impl set because it is implemented
    //exactly when sort for at is implemented
    string compFunc = "qsortComp_" + typeName + '_';
    //note: void* qsort(void* base, size_t num, size_t size, int (*compare) (const void*, const void*));
    Oss def;
    //don't need to forward-declare comp either, since only func will use it
    def << "int " << compFunc << "(const void* lhs, const void* rhs)\n{\n";
    def << "if(" << getLessFunc(t) << "((" << types[t];
    def << "*) lhs, ((" << types[t] << "*) rhs))\n";
    def << "return -1;\n";
    def << "else if(" << getEqualsFunc(t) << "((" << types[t];
    def << "*) lhs, ((" << types[t] << "*) rhs))\n";
    def << "return 0;";
    def << "else\nreturn 1;\n";
    def << "}\n";
    def << prototype.str();
    def << "\n{\n";
    def << "qsort(data, n, sizeof(void*), " << compFunc << ");\n";
    def << "}\n\n";
    utilFuncDefs << def.str();
    return func;
  }

  string getAccessFunc(ArrayType* at)
  {
    string& typeName = types[at];
    string func = "access_" + typeName + '_';
    if(accessImpl.find(at) != accessImpl.end())
    {
      return func;
    }
    accessImpl.insert(at);
    Oss prototype;
    Type* subtype = at->subtype;
    prototype << types[subtype] << "* " << func << '(' << typeName << "* arr, ";
    prototype << size_type << " index)";
    utilFuncDecls << prototype.str() << ";\n";
    Oss def;
    def << prototype.str() << "\n{\n";
    //note: size_type is unsigned so no need to check for >= 0
    def << "if(index < 0 || index >= arr->dim)\n{\n";
    def << "char buf[64];\n";
    def << "sprintf(buf, \"array index %u out of bounds\", index);\n";
    def << "panic(buf);\n";
    def << "}\n";
    def << "return " << getCopyFunc(at->subtype) << "(arr->data[index]);\n";
    def << "}\n\n";
    utilFuncDefs << def.str();
    return func;
  }

  string getAssignFunc(ArrayType* at)
  {
    string& typeName = types[at];
    string func = "assign_" + typeName + '_';
    if(assignImpl.find(at) != assignImpl.end())
    {
      return func;
    }
    assignImpl.insert(at);
    Oss prototype;
    Type* subtype = at->subtype;
    prototype << "void " << func << '(' << typeName << "* arr, ";
    prototype << types[subtype] << "* data, " << size_type << "* index)";
    utilFuncDecls << prototype.str() << ";\n";
    Oss def;
    def << prototype.str() << "\n{\n";
    //note: size_type is unsigned so no need to check for >= 0
    def << "if(index < 0 || index >= arr->dim)\n{\n";
    def << "char buf[64];\n";
    def << "sprintf(buf, \"array index %u out of bounds\", index);\n";
    def << "panic(buf);\n";
    def << "}\n";
    def << "arr->data[index] = " << getCopyFunc(subtype) << "(data);\n";
    def << "}\n\n";
    utilFuncDefs << def.str();
    return func;
  }

  string getHashFunc(Type* t)
  {
    string& typeName = types[t];
    string func = "hash_" + typeName + '_';
    if(hashImpl.find(t) != hashImpl.end())
    {
      return func;
    }
    hashImpl.insert(t);
    Oss prototype;
    prototype << "uint32_t " << func << "(void* val)";
    utilFuncDecls << prototype.str() << ";\n";
    Oss def;
    def << prototype.str() << "\n{\n";
    //starting value and multiplier for the 32-bit FNV-1a hash function
    const unsigned BASIS = 2166136261;
    const unsigned PRIME = 16777619;
    def << "uint32_t hash = " << BASIS << ";\n";
    if(t->isPrimitive())
    {
      //POD: can just deref each byte of the right size,
      //and statically generate the hash computation with each byte
      int size = 0;
      if(auto intType = dynamic_cast<IntegerType*>(t))
        size = intType->size;
      else if(auto floatType = dynamic_cast<FloatType*>(t))
        size = floatType->size;
      else if(dynamic_cast<EnumType*>(t))
        size = 8;
      else if(dynamic_cast<CharType*>(t))
        size = 1;
      else
      {
        INTERNAL_ERROR;
      }
      //generate an unwound loop that eats each byte of the primitive
      for(int i = 0; i < size; i++)
      {
        def << "hash ^= *(((char*) val) + " << i << ");\n";
        def << "hash *= " << PRIME << ";\n";
      }
    }
    else if(auto at = dynamic_cast<ArrayType*>(t))
    {
      def << types[t] << "* tmp = (" << types[t] << "*) val;\n";
      def << "for(" << size_type << " i = 0; i < tmp->dim; i++)\n{\n";
      //to hash an array, need to incorporate the indices into hash
      //i.e. hashes of [0, 1] and [1, 0] should be different
      def << "hash ^= (i * " << PRIME << " ^ " << getHashFunc(at->subtype) << "(tmp->data[i]));\n";
      def << "}\n";
    }
    else if(t->isStruct() || t->isTuple())
    {
      auto st = dynamic_cast<StructType*>(t);
      auto tt = dynamic_cast<TupleType*>(t);
      int numMems = 0;
      if(st)
        numMems = st->members.size();
      else
        numMems = tt->members.size();
      for(int i = 0; i < numMems; i++)
      {
        Type* memType = nullptr;
        if(st)
          memType = st->members[i]->type;
        else
          memType = tt->members[i];
        def << "hash ^= (" << i << "U * " << PRIME << " ^ ";
        def << getHashFunc(memType) << "(tmp->mem" << i << "));\n";
      }
    }
    else if(MapType* mt = dynamic_cast<MapType*>(t))
    {
      //since xor is associative and commutative,
      //order in which key-value pairs are hashed doesn't matter
      //Also, can use the hashes stored in buckets
      def << "HashTable* tab = (HashTable*) val;\n";
      def << "for(" << size_type << " i = 0; i < tab->numBuckets; i++)\n{\n";
      def << "for(" << size_type << " j = 0; j < tab->buckets[i]->size; j++)\n{\n";
      def << "hash ^= tab->buckets[i]->hashes[j] ^ " << getHashFunc(mt->value) << "(tab->buckets[i]->values[j]);\n";
      def << "}\n";
      def << "}\n";
    }
    def << "return hash;\n}\n\n";
    utilFuncDefs << def.str();
    return func;
  }

  void generateSectionHeader(ostream& c, string name)
  {
    c << "//////////////////////////////\n";
    int space = 13 - name.length() / 2;
    c << "//";
    for(int i = 0; i < space; i++)
      c << ' ';
    c << name;
    for(int i = 2 + space + name.length(); i < 28; i++)
      c << ' ';
    c << "//\n";
    c << "//////////////////////////////\n\n";
  }
}

