#ifndef C_GEN_H
#define C_GEN_H

#include "Misc.hpp"
#include "Utils.hpp"
#include "Parser.hpp"
#include <cstdio>
#include <cstring>

void genHeader(FILE* c, Parser::Module* ast);
void generateC(string outputStem, bool keep, Parser::Module* ast);

#endif

