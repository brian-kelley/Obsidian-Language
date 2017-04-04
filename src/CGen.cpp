#include "CGen.hpp"

using namespace std;

void generateC(string outputStem, bool keep, AP(Parser::ModuleDef)& ast)
{
  string cName = outputStem + ".c";
  string exeName = outputStem + ".exe";
  FILE* c = fopen(cName.c_str(), "wb");
  if(!c)
  {
    errAndQuit("Failed to open C file for writing.");
  }
  fprintf(c, "// ---%s, generated by the Onyx Compiler ---//\n\n", cName.c_str());
  genHeader(c, ast);
  fclose(c);
  //! feed into gcc
  string cmd = string("gcc ") + cName + " --std=c99 -o " + exeName;
  FILE* ccProcess = popen(cmd.c_str(), "r");
  //wait for cc to terminate
  int exitStatus = pclose(ccProcess);
  if(!keep)
  {
    remove(cName.c_str());
  }
  if(exitStatus)
  {
    errAndQuit("C compiler encountered error.");
  }
}

void genHeader(FILE* c, AP(Parser::ModuleDef)& ast)
{
  //Core language definitions (especially supporting primitives)

const char* header = 
  "#include \"stdlib.h\"\n"
  "#include \"stdio.h\"\n"
  "#include \"stdint.h\"\n"
  "#include \"string.h\"\n"

  "typedef struct\n"
  "{\n"
    "char* buf;\n"
    "u32 len;\n"
    "u32 cap;\n"
  "} string;\n"

  "string initEmptyString_()\n"
  "{\n"
    "string s;\n"
    "s.buf = malloc(16);\n"
    "s.len = 0;\n"
    "s.cap = 16;\n"
    "return s;\n"
  "}\n"

  "string initString_(const char* init, u32 len)\n"
  "{\n"
    "string s;\n"
    "s.cap = len > 16 ? len : 16;\n"
    "s.buf = malloc(s.cap);\n"
    "memcpy(s.buf, init, len);\n"
    "s.len = len;\n"
    "return s;\n"
  "}\n"

  "void appendString_(string* lhs, string* rhs)\n"
  "{\n"
    "u32 newLen = lhs->len + rhs->len;\n"
    "if(newLen > lhs->cap)\n"
    "{\n"
      "lhs->buf = realloc(lhs->buf, newLen);\n"
      "lhs->cap = newLen;\n"
    "}\n"
    "memcpy(lhs->buf + lhs->len, rhs->buf, rhs->len);\n"
    "lhs->len = newLen;\n"
  "}\n"

  "void prependStringLiteral_(string* str, const char* literal, u32 len)\n"
  "{\n"
    "u32 newLen = str->len + len;\n"
    "if(newLen > str->cap)\n"
    "{\n"
      "str->buf = realloc(str->buf, newLen);\n"
      "str->cap = newLen;\n"
    "}\n"
    "memcpy(str->buf + str->len, str->buf, str->len);\n"
    "memcpy(str->buf, literal, len);\n"
    "str->len = newLen;\n"
  "}\n"

  "void appendStringLiteral_(string* str, const char* literal, u32 len)\n"
  "{\n"
    "u32 newLen = str->len + len;\n"
    "if(newLen > str->cap)\n"
    "{\n"
      "str->buf = realloc(str->buf, newLen);\n"
      "str->cap = newLen;\n"
    "}\n"
    "memcpy(str->buf + str->len, literal, len);\n"
    "str->len = newLen;\n"
  "}\n"

  "void appendStringChar_(string* str, char c)\n"
  "{\n"
    "if(str->len == str->cap)\n"
    "{\n"
      "str->buf = realloc(str->buf, str->cap + 1);\n"
      "str->cap++;\n"
    "}\n"
    "str->buf[str->len] = c;\n"
    "str->len++;\n"
  "}\n"

  "string disposeString_(string* s)\n"
  "{\n"
    "free(s->buf);\n"
  "};\n";

  fputs(header, c);
}

