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

  struct PointerType : public Datatype
  {
    Datatype* t;
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

  struct Value
  {
    virtual bool modifiable() = 0;
    Datatype* type;
  };

  //Unknown is only used in constant propagation,
  //representing when nothing is known about a value
  struct Unknown : public Value
  {
  };

  //A read/write reference to a variable
  struct Ref : public Value
  {
    Var* var;
  };

  //Reference to a member of a fixed-size structure
  //(so idx is always known at compile-time)
  struct Member : public Value
  {
    Value* root;
    int idx; 
  };

  //Index into an arbitrary-sized array, with
  //a runtime index
  struct Index : public Value
  {
    Value* ptr;
    Value* idx;
  };

  struct IntVal : public Value
  {
    uint64_t val;  //static-cast to actual type
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
    //Index within function
    int index;
  };

  struct BasicBlock
  {
    vector<BasicBlock*> in;
    vector<BasicBlock*> out;
    int begin;
    int end;
  };

  struct Func
  {
    vector<Instruction*> instr;
    vector<BasicBlock*> blocks;
  };

  struct Nop : public Instruction
  {
  };

  //extern static Nop* nop;

  //Shallow copy a value
  struct Assign : public Instruction
  {
    Value* src;
    Value* dst;
  };

  //Deep copy a value.
  //If src/dst are PointerType, then dst is
  //allocated to fit the size of src and then elements
  //are copied. If src is a compound data type,
  //all heap blocks owned are copied in this way.
  struct Copy : public Instruction
  {
    Value* src;
    Value* dst;
  };

  //Deep copy a value, while changing its type
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

  //Malloc allocates a heap block.
  //Either a single element (n = NULL, used for unions),
  //or for a contiguous array.
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
  
  //Control flow
  struct Label : public Instruction
  {
  };
  
  struct Jump : public Instruction
  {
    Label* target;
  };

  struct CondJump : public Instruction
  {
    Value* cond;
    //True = fall through, false = goto falseTarget 
    Label* falseTarget;
  };
}

#endif

