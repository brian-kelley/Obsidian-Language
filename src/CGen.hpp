#ifndef C_GEN_H
#define C_GEN_H

#include "Misc.hpp"
#include "Utils.hpp"
#include "Type.hpp"
#include "Parser.hpp"
#include <cstdio>

void genHeader(FILE* c, AP(Parser::ModuleDef)& ast);
void generateC(string outputStem, bool keep, AP(Parser::ModuleDef)& ast);

#endif

