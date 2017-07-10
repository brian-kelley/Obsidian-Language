#include "Variable.hpp"

unsigned Variable::nextID = 1;

Variable(Scope* s, Parser::VarDecl* astIn) : name(astIn->name), scope(s), ast(astIn), id(nextID++)
{
  //find type (this must succeed or is an error)
  //failureIsError true, so TypeSystem will do the error
  type = TypeSystem::getType(ast->type.get(), scope, nullptr, true);
}

