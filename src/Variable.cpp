#include "Variable.hpp"

Variable::Variable(Scope* s, Parser::VarDecl* ast)
{
  name = ast->name;
  scope = s;
  isStatic = ast->isStatic;
  if(isStatic && !dynamic_cast<StructScope*>(s))
  {
    //tried to make static var which is not directly member of struct
    ERR_MSG("Tried to declare var \"" + name + "\" in scope \"" +
        s->getLocalName() + "\" static, but scope is not a struct.");
  }
  //find type (this must succeed)
  //failureIsError true, so TypeSystem will produce the error
  type = TypeSystem::getType(ast->type, scope, NULL, true);
}

