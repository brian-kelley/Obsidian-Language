#ifndef CPP_GEN_H
#define CPP_GEN_H

#include "Misc.hpp"
#include "Utils.hpp"
#include "Type.hpp"

void generateCPP(string outputStem, bool keep, string& code);
void generateTypeHeader(FILE* cpp, string& code);
void generateFuncHeader(FILE* cpp, string& code);
void compileCPP(string cppSrc, string output);

#endif

