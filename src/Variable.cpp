#include "Variable.hpp"
#include "Subroutine.hpp"

static int nextVarID = 0;

Variable::Variable(Scope* s, string n, Type* t, Expression* init, bool isStatic, bool compose)
{
  scope = s;
  name = n;
  type = t;
  owner = s->getMemberContext();
  initial = init;
  //if this variable is nonstatic and is inside a struct, add it as member
  if(!isStatic && owner)
  {
    //add this as a member of the struct
    owner->members.push_back(this);
    owner->composed.push_back(compose);
  }
  id = nextVarID++;
}

Variable::Variable(string n, Type* t, Block* b)
{
  scope = b->scope;
  name = n;
  type = t;
  //initial values in local variables are handled separately,
  //by inserting an Assign statement at point of declaration
  initial = nullptr;
  owner = nullptr;
  id = nextVarID++;
}

void Variable::resolveImpl()
{
  resolveType(type);
  if(!initial)
  {
    //Always need an initial expression, so use the
    //default one implicitly
    initial = type->getDefaultValue();
  }
  resolveExpr(initial);
  if(!type->canConvert(initial->type))
    errMsgLoc(this, "cannot convert from " << initial->type->getName() << " to " << type->getName());
  //convert initial value, if necessary
  if(initial->type != type)
    initial = new Converted(initial, type);
  resolved = true;
}

bool Variable::isParameter()
{
  return scope->node.is<Subroutine*>();
}

bool Variable::isGlobal()
{
  return !isParameter() && !isLocal() && !isMember();
}

bool Variable::isLocal()
{
  return scope->node.is<Block*>();
}

bool Variable::isMember()
{
  return owner;
}

