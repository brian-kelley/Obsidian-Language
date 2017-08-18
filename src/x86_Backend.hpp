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
  struct Integer : public NativeType
  {
    int size; //1, 2, 4 or 8
    bool isSigned;
  };
  struct Float : public NativeType
  {
    int size; //4 or 8
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
  struct StackFrame
  {
    //bytes of space in the stack frame
    size_t bytes;
    //Saved integer regs, in order of push
    int saved[16];
    //Number of saved general purpose regs
    int numSaved;
  };
  //Code generation
  string getNewSymbol();
  void implSubroutine(Subroutine* s, Oss& assembly);
  void openStackFrame(Oss& assembly);
  void closeStackFrame(Oss& assembly);
  //lazily get the native concrete type for given abstract type
  NativeType* getNativeType(TypeSystem::Type* t);
}

#endif
/*
 * Backend plan:
 * -Machine representations of all Types
 *    -Integer, float, bool and void types are easy
 *    -Struct and tuple are structures with aligned members
 *      -All members must have constant size
 *    -Arrays: store dimension counts in 4-byte unsigned, then data
 *      -for now, arrays are always malloc'd so they have a fixed size
 *      -this is a pointer
 *    -All variables must have a place on the stack because temporary expression evaluation will use registers
 *
 * -Values have machine types, have a single, fixed-size location
 *    -location can be a register, stack offset, heap block + offset or data segment fixed offset
 *
 * -Place literals in data segment, except ints/bools (can be put in regs or stack directly)
 *
 * -compute Expressions bottom-up by doing arithmetic operations on values
 *    -can use the stack redzone as temporary space, but prefer registers that are available
 *
 * -implement subroutines:
 *    -follow System V ABI for all arguments
 *    -compound values are passed as pointers to value (caller prepares)
 *    -this means return vals are in rax, but size not known by caller so callee allocates it
 *    -don't emit stack frame prep immediately: need to determine set of registers to be preserved
 */

