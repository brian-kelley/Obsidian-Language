#ifndef X86_BACKEND
#define X86_BACKEND

#include "MiddleEnd.hpp"
#include <unordered_map>
#include <sstream>

extern ModuleScope* global;

typedef ostringstream Oss;

namespace x86
{
  //Generate 64-bit x86 assembly for whole program
  //in human-readable Intel syntax (in memory)
  string generateAsm();
  //Write assembly to file and run the assembler command (does disk I/O)
  //Also link with libc and libm
  void buildExecutable(string& code, bool keepAsm, string& nameStem);
  string getObjFormat();
  enum INT_REGS
  {
    RAX,
    RBX,
    RCX,
    RDX,
    RSP,
    RBP,
    RSI,
    RDI,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15
  };
  //Native information about primitive types
  //Native memory layout of compound types (struct, tuple)
  struct NativeType
  {
    virtual ~NativeType() {}
  };
  struct Primitive : public NativeType
  {
    enum RegType
    {
      INT_REG,
      SSE_REG
    };
  };
  struct Compound : public NativeType
  {
    vector<NativeType*> members;
    size_t offsetOf(int member);
    size_t size();
  };
  struct LocalVar
  {
    LocalVar(size_t stackOffset, NativeType* natType);
    size_t stackOffset; //offset relative to rbp
    NativeType* natType;
  };
  //Code generation
  string getNewSymbol();
  void implSubroutine(Subroutine* s, Oss& assembly);
  void beginSubroutine(Oss& assembly);
  void endSubroutine(Oss& assembly);
  //lazily get the native concrete type for given abstract type
  NativeType* getNativeType(TypeSystem::Type* t);
}

#endif

