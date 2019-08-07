#ifndef IR_H
#define IR_H

#include "Common.hpp"
#include "Subroutine.hpp"

namespace IR
{
  struct Datatype
  {
    bool ownsMemory;
  };

  enum struct IntType
  {
    u8,
    i8,
    u16,
    i16,
    u32,
    i32,
    u64,
    i64
  };

  struct Integer : public Datatype
  {
    IntType t;
  };

  enum struct FloatType
  {
    f32,
    f64
  };

  struct Float : public Datatype
  {
    FloatType t;
  };

  struct Compound : public Datatype
  {
    vector<Datatype*> elems;
  };

  struct Memory : public Datatype
  {
    Datatype* elem;
  };

  //Fixed-size storage of a given type
  struct Var
  {
    Datatype* type;
  };

  /*
    Instruction types:
      -Copy
      -Convert
      -UnArith
      -BinArith
      -DirectCall
      -IndirectCall
      -Malloc
      -Free
  */

  struct Value
  {
  };

  struct Ref : public Value
  {
    Var* var;
  };

  struct Member : public Value
  {
    Value* root;
    int idx; 
  };

  struct IntVal : public Value
  {
    u64 val;  //static-cast to actual type
  };

  struct FloatVal : public Value
  {
    double val;
  };

  struct CompoundVal : public Value
  {
    vector<Value*> elems;
  };

  struct Instruction
  {
    virtual bool hasSideEffects()
    {
      return false;
    }
  };

  struct Copy : public Instruction
  {
    Value* src;
    Value* dst;
  };

  struct Convert : public Instruction
  {
    Value* src;
    Value* dst;
  };

  struct DirectCall : public Instruction
  {
    Func* callable;
    Value* thisObj;
    vector<Value*> args;
  };

  struct IndirectCall : public Instruction
  {
    Value* callable;
    vector<Value*> args;
  };

  struct Malloc : public Instruction
  {
    Value* n;
    Datatype* elem;
    Value* dst;
  };

  struct Free : public Instruction
  {
    Value* v;
  };

  enum struct UnOp
  {
    BIT_NOT,
    LOG_NOT,
    NEG
  };

  struct UnArith : public Instruction
  {
    Value* dst;
    Value* src;
    UnOp op;
  };

  enum struct BinOp
  {
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    LOG_OR,
    LOG_AND,
    BIN_OR,
    BIN_AND,
    BIN_XOR,
    SHL,
    SHR,
    CMP_EQ,
    CMP_NEQ,
    CMP_L,
    CMP_LE,
    CMP_G,
    CMP_GE
  };

  struct BinArith : public Instruction
  {
    Value* out;
    Value* in1;
    Value* in2;
    BinOp op;
  };
}

#endif

