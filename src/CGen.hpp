#ifndef C_GEN_H
#define C_GEN_H

#include "Misc.hpp"
#include "Utils.hpp"
#include "Type.hpp"
#include "Parser.hpp"
#include <cstdio>
#include <cstring>

void genHeader(FILE* c, AP(Parser::Module)& ast);
void generateC(string outputStem, bool keep, AP(Parser::Module)& ast);

#endif

