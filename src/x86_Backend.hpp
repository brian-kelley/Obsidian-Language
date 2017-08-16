#ifndef X86_BACKEND
#define X86_BACKEND

#include "MiddleEnd.hpp"

extern ModuleScope* global;

namespace x86
{
  //Generate 64-bit x86 assembly for whole program
  //in human-readable Intel syntax (in memory)
  string generateAsm();
  //Write assembly to file and run the assembler command (does disk I/O)
  //Also link with libc and libm
  void buildExecutable(string& code, bool keepAsm, string& nameStem);
}

#endif

