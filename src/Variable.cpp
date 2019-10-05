#include "Variable.hpp"
#include "Subroutine.hpp"

static int nextVarID = 0;

Variable::Variable(Scope* s, string n, Type* t, Expression* init, bool isStatic, bool compose)
{
  scope = s;
  name = n;
  type = t;
  owner = s->getMemberContext();
  if(isStatic && !owner)
    errMsgLoc(this, "variable declared static outside a struct");
  if(isStatic)
    owner = nullptr;
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
  scope = b ? b->scope : nullptr;
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
  INTERNAL_ASSERT(type->resolved);
  if(initial)
  {
    resolveExpr(initial);
    INTERNAL_ASSERT(initial->resolved);
    if(!type->canConvert(initial->type))
      errMsgLoc(this, "cannot convert from " << initial->type->getName() << " to " << type->getName());
    //convert initial value, if necessary
    if(!typesSame(initial->type, type))
      initial = new Converted(initial, type);
  }
  else
    initial = type->getDefaultValue();
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

bool Variable::isLocalOrParameter()
{
  return isLocal() || isParameter();
}

bool Variable::isMember()
{
  return owner;
}

