#include "CppGen.hpp"

void generateCommon(FILE* cpp)
{
  fputs(
     "#include <iostream>\n"
     //Style 1 int types
     "typedef unsigned char uchar; "
     "typedef unsigned short ushort; "
     "typedef unsigned int uint; "
     "typedef unsigned long ulong; "
     //Style 2 int types
     "typedef char s8; "
     "typedef short s16; "
     "typedef int s32; "
     "typedef long s64; "
     "typedef unsigned char u8; "
     "typedef unsigned short u16; "
     "typedef unsigned int u32; "
     "typedef unsigned long u64; "
     //Define string type & implementation
    , cpp);
}

void generateCPP(string filename, string& code)
{
  FILE* cpp = fopen(filename.c_str(), "wb");
  if(!out)
  {
    errAndQuit("Failed to open CPP file for writing.");
  }
  generateTypeHeader(cpp, code);
  generateFuncHeader(cpp, code);
  fclose(cpp);
}

void generateTypeHeader(FILE* cpp, string& code)
{
}

void generateFuncHeader(FILE* cpp, string& code)
{
}

