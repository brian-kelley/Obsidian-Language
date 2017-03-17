#ifndef C_GEN_H
#define C_GEN_H

#include "Misc.hpp"
#include "Utils.hpp"
#include "Type.hpp"
#include "Parser.hpp"

void generateC(string outputStem, bool keep, Program p);
void generateTypeHeader(FILE* c, string& code);
void generateFuncHeader(FILE* c, string& code);
void generateCode(FILE* c, Program& prog);

#endif

