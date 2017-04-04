#include "MiddleEnd.hpp"

using namespace std;
using namespace Parser;
using namespace MiddleEnd;

void MiddleEnd::semanticCheck(AP(ModuleDef)& ast)
{
  checkEntryPoint(ast);
}

void MiddleEnd::checkEntryPoint(AP(ModuleDef)& ast)
{
  for(auto& decl : ast->decls)
  {
    if(decl.decl.which() == 11)
    {
      auto& procDef = decl.decl.get<UP(ProcDef)>();
      if(procDef->
    }
  }
}

