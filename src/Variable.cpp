#include "Variable.hpp"

Variable::Variable(Scope* s, Parser::VarDecl* ast)
{
  name = ast->name;
  this->scope = s;
  this->isStatic = ast->isStatic;
  if(isStatic && !dynamic_cast<StructScope*>(s))
  {
    //tried to make static var which is not directly member of struct
    ERR_MSG("Tried to declare var \"" + name + "\" in scope \"" +
        s->getLocalName() + "\" static, but scope is not a struct.");
  }
  //find type (this must succeed)
  //failureIsError true, so TypeSystem will produce the error
  this->type = TypeSystem::getType(ast->type, scope, NULL, true);
}

Variable::Variable(Scope* s, string name, TypeSystem::Type* t)
{
  this->name = name;
  this->scope = s;
  this->isStatic = false;
  this->type = t;
}

