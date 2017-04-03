#ifndef C_GEN_H
#define C_GEN_H

#include "Misc.hpp"
#include "Utils.hpp"
#include "Type.hpp"
#include "Parser.hpp"
#include <cstdio>

void generateC(string outputStem, bool keep, AP(Parser::ModuleDef)& ast);

#endif

