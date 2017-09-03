#include "x86_Backend.hpp"
#include <cstdio>

static size_t symbolCounter = 0;

unordered_map<Subroutine*, string> textSyms;
unordered_map<Variable*, string> dataSyms;

namespace x86
{
  string generateAsm()
  {
    return "";
  }

  void buildExecutable(string& code, bool keepAsm, string& nameStem)
  {
    FILE* asmFile;
    string asmFileName = nameStem + ".asm";
    string objFileName = nameStem + ".o";
    //TODO: delete assembly file if keep assembly wasn't enabled in argv
    asmFile = fopen(asmFileName.c_str(), "w");
    fwrite(code.c_str(), 1, code.length(), asmFile);
    fclose(asmFile);
    //run the assembler (nasm) on the assembly file
    //then use the "gcc" to link object with the C lib/runtime to get exe
    if(!runCommand("nasm -f" + getObjFormat() + ' ' + asmFileName + " -o " + objFileName))
    {
      INTERNAL_ERROR;
    }
    //TODO: replace "gcc" with ld, linking in libc, libm and crt
    //that involves replacing main with _start as entry point symbol
    if(!runCommand("gcc " + objFileName + " -o " + nameStem))
    {
      INTERNAL_ERROR;
    }
  }

  string getObjFormat()
  {
#ifdef __APPLE__
    return "macho64";
#elif __linux
    return "elf64";
#elif _WIN32
    return "win64";
#else
    ERR_MSG("Unknown platform (not win/mac/linux) for native, local object format.");
    return "";
#endif
  }

  string getNewSymbol()
  {
    //get symbolCounter in base-26 (reversed, but that doesn't matter)
    string sym;
    size_t val = symbolCounter;
    while(val)
    {
      size_t digit = val % 26;
      sym += ('a' + digit);
      val /= 26;
    }
    symbolCounter++;
    return sym;
  }

  void implSubroutine(Subroutine* s, Oss& assembly)
  {
    //get label symbol name
    string symName = getNewSymbol();
    assembly << symName << ":\n";
    openStackFrame(assembly);
    //lazily generate the native types for all local variables
    //note: everything in a block scope is local (stack)
    //size_t stackCounter = 0;
    //auto scope = s->body->scope;
    closeStackFrame(assembly);
    assembly << "ret\n\n";
  }

  void openStackFrame(Oss& assembly)
  {
    assembly << "push rbp\n";
    assembly << "mov rbp, rsp\n";
  }

  void closeStackFrame(Oss& assembly)
  {
    assembly << "pop rbp\n";
  }
}

