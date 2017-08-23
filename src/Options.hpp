#ifndef OPTIONS_H
#define OPTIONS_H

#include "Common.hpp"
#include "string.h"

struct Options
{
  enum BackendType
  {
    BACKEND_C,
    BACKEND_LLVM,
    BACKEND_X86
  };
  string input;
  string outputStem;
  int backend;
  //emit intermediate c source
  bool emitC;
  //emit llvm ir
  bool emitLLVM;
  //emit x86_64 assembly
  bool emitX86;
};

Options getDefaultOptions();
Options parseOptions(int argc, const char** argv);

#endif

