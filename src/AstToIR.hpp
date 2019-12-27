#ifndef AST_TO_IR_H
#define AST_TO_IR_H

#include "Common.hpp"
#include "IR.hpp"
#include "TypeSystem.hpp"
#include "Expression.hpp"
#include "Subroutine.hpp"

/*
namespace AstToIR
{
  using IR::Datatype;
  using IR::Value;
  
  struct IRBuilder
  {
    IRBuilder();
    string getTempIdentifier();
    int tempCounter;
  };

  struct ArrayStructure
  {
    virtual Datatype* getType() = 0;
    virtual Value* genAlloc(vector<Value*>& dims) = 0;
    //output arr[ind] = val;
    virtual void genWrite(Value* arr, Value* ind, Value* val) = 0;
    //get arr[ind] as a value
    virtual Value* genRead(Value* arr, Value* ind) = 0;
    //append single element
    virtual Value* genAppendElem(Value* arr, Value* elem) = 0;
    //append another array (with the same datatype)
    virtual Value* genAppendArr(Value* arr, Value* toAppend) = 0;
    virtual void genFree(Value* arr) = 0;
  };

  struct MapStructure
  {
    virtual Datatype* getType() = 0;
    virtual Value* genCreate() = 0;
    virtual void genInsert(Value* m, Value* key, Value* val) = 0;
    virtual void genRemove(Value* m, Value* key) = 0;
    virtual Value* genLookup(Value* m, Value* key) = 0;
    virtual void genFree(Value* m) = 0;
  };

  //Data structure implementations
  struct VectorArray : public ArrayStructure
  {
    Datatype* getType();
    Value* genAlloc(vector<Value*>& dims);
    //output arr[ind] = val;
    void genWrite(Value* arr, Value* ind, Value* val);
    //get arr[ind] as a value
    Value* genRead(Value* arr, Value* ind);
    //append single element
    Value* genAppendElem(Value* arr, Value* elem);
    //append another array (with the same datatype)
    Value* genAppendArr(Value* arr, Value* toAppend);
    void genFree(Value* arr);
    Datatype* type;
  };

  struct SimpleHashTable : public MapStructure
  {
    Datatype* getType();
    Value* genCreate();
    void genInsert(Value* m, Value* key, Value* val);
    void genRemove(Value* m, Value* key);
    Value* genLookup(Value* m, Value* key);
    void genFree(Value* m);

    Datatype* type;
  };

  struct RedBlackTree : public MapStructure
  {
    Datatype* getType();
    Datatype* type;
  };
}
*/

#endif

