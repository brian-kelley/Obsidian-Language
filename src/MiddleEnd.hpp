#ifndef MIDDLE_END_H
#define MIDDLE_END_H

#include "Misc.hpp"
#include "Utils.hpp"
#include "Variable.hpp"
#include "Parser.hpp"
#include "TypeSystem.hpp"

namespace MiddleEnd
{
  void semanticCheck(AP(Parser::ModuleDef)& ast);
  void checkEntryPoint(AP(Parser::ModuleDef)& ast);
}

#endif

