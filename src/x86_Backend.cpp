#include "x86_Backend.hpp"
#include <cstdio>

namespace x86
{
  string generateAsm()
  {
    return "";
  }

  void buildExecutable(string& code, bool keepAsm, string& nameStem)
  {
    FILE* asmFile;
    string asmFileName;
    if(keepAsm)
    {
      asmFileName = nameStem + ".asm";
    }
    else
    {
      asmFileName = tmpnam(NULL);
    }
    asmFile = fopen(asmFileName.c_str(), "w");
    fwrite(code.c_str(), 1, code.length(), asmFile);
    fclose(asmFile);
    //run the assembler on 
  }
}
