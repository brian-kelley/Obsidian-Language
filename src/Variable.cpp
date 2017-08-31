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
  this->type = TypeSystem::lookupType(ast->type, scope);
  if(!this->type)
  {
    errAndQuit("variable " + name + " type could not be determined");
  }
}

Variable::Variable(Scope* s, string n, TypeSystem::Type* t)
{
  this->name = n;
  this->scope = s;
  this->isStatic = false;
  this->type = t;
}

