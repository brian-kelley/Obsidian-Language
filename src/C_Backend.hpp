#ifndef C_GEN_H
#define C_GEN_H

#include "IR.hpp"

/* C backend memory management:
 *
 *  -global variables are statically allocated
 *  -local variables are stack allocated
 *  -array, map and union storage are heap allocated
 *  -primitives/enums passed by value; everything else by pointer
 */

namespace C
{
  void init();

  struct CType
  {
    virtual string getName() = 0;
    virtual int getKindTag() = 0;
    virtual bool isVoid() {return false;}
  };

  struct CPrim : public CType
  {
    enum
    {
      VOID,
      BOOL,
      I8,
      U8,
      I16,
      U16,
      I32,
      U32,
      I64,
      U64,
      F32,
      F64,
      NUM_TYPES
    };
    CPrim(int t) : type(t) {}
    string getName()
    {
      switch(type)
      {
        case VOID: return "void";
        case BOOL: return "bool";
        case I8:   return "int8_t";
        case U8:   return "uint8_t";
        case I16:  return "int16_t";
        case U16:  return "uint16_t";
        case I32:  return "int32_t";
        case U32:  return "uint32_t";
        case I64:  return "int64_t";
        case U64:  return "uint64_t";
        case F32:  return "float";
        case F64:  return "double";
        default:;
      }
      INTERNAL_ERROR;
      return "";
    }
    int getKindTag()
    {
      return 0;
    }
    bool isVoid()
    {
      return type == VOID;
    }
    int type;
  };

  bool operator==(const CPrim& p1, const CPrim& p2);
  bool operator<(const CPrim& p1, const CPrim& p2);

  struct CStruct : public CType
  {
    CStruct(StructType* st);
    CStruct(TupleType* tt);
    vector<CType*> mems;
    string getName()
    {
      return getIdent(id);
    }
    int getKindTag()
    {
      return 1;
    }
    int id;
  };

  bool operator==(const CStruct& s1, const CStruct& s2);
  bool operator<(const CStruct& s1, const CStruct& s2);

  struct CArray : public CType
  {
    CArray(ArrayType* at);
    CType* subtype;
    string getName()
    {
      return getIdent(id);
    }
    int getKindTag()
    {
      return 2;
    }
    int id;
  };

  bool operator==(const CArray& a1, const CArray& a2);
  bool operator<(const CArray& a1, const CArray& a2);

  struct CUnion : public CType
  {
    string getName()
    {
      return "Union";
    }
    int getKindTag()
    {
      return 3;
    }
  };

  struct CMap : public CType
  {
    CMap(MapType* mt);
    CType* key;
    CType* value;
    string getName()
    {
      return getIdent(id);
    }
    int id;
  };

  bool operator==(const CMap& m1, const CMap& m2);
  bool operator<(const CMap& m1, const CMap& m2);

  struct CCallable : public CType
  {
    CCallable(CallableType* ct);
    CCallable(Subroutine* subr);
    //both constructors call this
    void init(CallableType* ct);
    //the type of the "this" struct (can be null)
    //in C, a pointer to that type is the 1st real argument
    CStruct* thisType;
    CType* returnType;
    //note: args only includes the types that aren't "void" in C
    vector<CType*> args;
    string getName()
    {
      return getIdent(id);
    }
    int id;
  };

  bool operator==(const CCallable& c1, const CCallable& c2);
  bool operator<(const CCallable& c1, const CCallable& c2);

  //Comparator (for std::set) implements operator<
  struct CTypeCompare
  {
    bool operator()(const CType* t1, const CType* t2);
  };

  struct CTypeEqual
  {
    bool operator()(const CType* t1, const CType* t2);
  };

  //Get a canonical C representation of the type
  CType* getCType(Type* t);
  string getTypeName(Type* t);
  string getTypeName(CType* c);

  //Generate C source file to outputStem.c, then run C compiler, and if !keep delete the source file
  void generate(string outputStem, bool keep);
  //add file label and basic libc includes
  void genCommon();
  //generate generic map (hash table) implementation
  //this goes in utilFuncDecls/Defs
  void implHashTable();
  //generate these core builtin subroutines that can't be written in Onyx
  //(necessary for real compiler):
  //  proc ubyte[] readFile(char[] filename)
  //  proc void writeFile(ubyte[] data, char[] filename)
  //  proc char[] readLine()
  //  proc char[] readToken()
  //  func uint floatRepr(float f)
  //  func ulong doubleRepr(double d)
  //
  //  Note: following builtins are implemented in Onyx (see BuiltIn.cpp)
  //
  //  func long? stoi(char[] str)
  //  func double? stod(char[] str)
  //  func char[] printHex(ulong num)
  //  func char[] printBin(ulong num)
  void genCoreBuiltins();
  //forward-declare all compound types (and arrays),
  //then actually define them as C structs
  void genTypeDecls();
  //declare all global/static data
  void genGlobals();
  //forward-declare all subroutines, then actually provide impls
  void genSubroutines();
  void genMain(Subroutine* m);

  //Generate a unique integer to be used as C identifier
  //Convert integer to unique C identifier
  string getIdent(int i);
  string getIdentifier();

  //generate a nicely formatted "section" header in a comment
  void generateSectionHeader(ostream& c, string name);

  void emitStatement(ostream& c, IR::StatementIR* stmt);

  //utility functions
  //lazily generate and return name of C function
  //(also generates all other necessary util funcs)
  string getInitFunc(Type* t);
  string getCopyFunc(Type* t);
  //alloc functions take N integers and produce N-dimensional rectangular array
  string getAllocFunc(ArrayType* t);
  string getDeallocFunc(Type* t);
  string getPrintFunc(Type* t);
  //convert and deep-copy input from one type to another
  //precondition: out->canConvert(in)
  string getConvertFunc(Type* out, Type* in);
  //compare two inputs for equality
  string getEqualsFunc(Type* t);
  //test first < second
  string getLessFunc(Type* t);
  //given two arrays, return a new concatenated array
  string getConcatFunc(ArrayType* at);
  string getAppendFunc(ArrayType* at);
  string getPrependFunc(ArrayType* at);
  //generate "void sort_T_(T** data, size_type n)"
  string getSortFunc(Type* t);
  //given an array and an index, "safely" access the element
  //program terminates if out of bounds
  string getAccessFunc(ArrayType* at);
  string getAssignFunc(ArrayType* at);
  string getHashFunc(Type* t);
  string getHashInsert(Type* t);

  //true if type owns heap memory
  bool typeNeedsDealloc(Type* t);
  //true if type is stack-allocated and passed by value
  bool isPOD(Type* t);
  //true if type needs "->" to access members instead of "."
  bool isPointer(Expression* expr);
}

#endif

