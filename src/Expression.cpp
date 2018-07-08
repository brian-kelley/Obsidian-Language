#include "Expression.hpp"
#include "Variable.hpp"
#include "Scope.hpp"
#include "Subroutine.hpp"

/**************
 * UnaryArith *
 **************/

UnaryArith::UnaryArith(int o, Expression* e)
  : op(o), expr(e) {}

void UnaryArith::resolveImpl(bool final)
{
  resolveExpr(expr, final);
  if(expr->resolved)
  {
    if(op == LNOT && expr->type != primitives[Prim::BOOL])
    {
      errMsgLoc(this, "! operand must be a bool");
    }
    else if(op == BNOT && !expr->type->isInteger())
    {
      errMsgLoc(this, "~ operand must be an integer");
    }
    else if(op == SUB && !expr->type->isNumber())
    {
      errMsgLoc(this, "unary - operand must be a number");
    }
    else
    {
      //any other operator can't be parsed as unary
      INTERNAL_ERROR;
    }
    type = expr->type;
    resolved = true;
  }
}

/***************
 * BinaryArith *
 ***************/

BinaryArith::BinaryArith(Expression* l, int o, Expression* r) : op(o), lhs(l), rhs(r) {}

void BinaryArith::resolveImpl(bool final)
{
  cout << "Resolving binary arith with operator " << operatorTable[op] << '\n';
  resolveExpr(lhs, final);
  resolveExpr(rhs, final);
  if(!lhs->resolved || !rhs->resolved)
  {
    return;
  }
  //Type check the operation
  auto ltype = lhs->type;
  auto rtype = rhs->type;
  cout << "REsolving binary arith with lhs type " << lhs->type->getName() << " and rhs type " << rhs->type->getName() << '\n';
  switch(op)
  {
    case LOR:
    case LAND:
    {
      if(ltype != primitives[Prim::BOOL] ||
         rtype != primitives[Prim::BOOL])
      {
        errMsgLoc(this, "operands to " << operatorTable[op] << " must be bools.");
      }
      //type of expression is always bool
      this->type = primitives[Prim::BOOL];
      break;
    }
    case BOR:
    case BAND:
    case BXOR:
    {
      //both operands must be integers
      if(!(ltype->isInteger()) || !(rtype->isInteger()))
      {
        errMsgLoc(this, "operands to " << operatorTable[op] << " must be integers.");
      }
      //the resulting type is the wider of the two integers, favoring unsigned
      type = promote(ltype, rtype);
      if(ltype != type)
      {
        lhs = new Converted(lhs, type);
      }
      if(rtype != type)
      {
        rhs = new Converted(rhs, type);
      }
      break;
    }
    case PLUS:
    {
      //intercept plus operator for arrays (concatenation, prepend, append)
      auto lhsAT = dynamic_cast<ArrayType*>(ltype);
      auto rhsAT = dynamic_cast<ArrayType*>(rtype);
      if(lhsAT && rhsAT)
      {
        if(rhsAT->canConvert(lhsAT))
        {
          type = ltype;
        }
        else if(lhsAT->canConvert(rhsAT))
        {
          type = rtype;
        }
        else
        {
          errMsgLoc(this, "incompatible array concatenation operands: " << ltype->getName() << " and " << rtype->getName());
        }
        if(ltype != type)
        {
          lhs = new Converted(lhs, type);
        }
        if(rtype != type)
        {
          rhs = new Converted(rhs, type);
        }
        break;
      }
      else if(lhsAT)
      {
        //array append
        Type* subtype = lhsAT->subtype;
        if(!subtype->canConvert(rtype))
        {
          errMsgLoc(this, "can't append type " << rtype->getName() << " to " << ltype->getName());
        }
        type = ltype;
        if(subtype != rtype)
        {
          rhs = new Converted(rhs, subtype);
        }
        break;
      }
      else if(rhsAT)
      {
        //array prepend
        Type* subtype = rhsAT->subtype;
        if(!subtype->canConvert(rtype))
        {
          errMsgLoc(this, "can't prepend type " << ltype->getName() << " to " << rtype->getName());
        }
        type = rtype;
        if(subtype != ltype)
        {
          lhs = new Converted(lhs, subtype);
        }
        break;
      }
    }
    case SUB:
    case MUL:
    case DIV:
    case MOD:
    {
      //TODO (CTE): error for div/mod with rhs = 0
      //TODO: support array concatenation with +
      if(!(ltype->isNumber()) || !(rtype->isNumber()))
      {
        errMsgLoc(this, "operands to arithmetic operators must be numbers.");
      }
      type = promote(ltype, rtype);
      if(ltype != type)
      {
        lhs = new Converted(lhs, type);
      }
      if(rtype != type)
      {
        rhs = new Converted(rhs, type);
      }
      break;
    }
    case SHL:
    case SHR:
    {
      //TODO (CTE): error for rhs < 0
      if(!(ltype->isInteger()) || !(rtype->isInteger()))
      {
        errMsgLoc(this, "operands to " << operatorTable[op] << " must be integers.");
      }
      type = ltype;
      break;
    }
    case CMPEQ:
    case CMPNEQ:
    case CMPL:
    case CMPLE:
    case CMPG:
    case CMPGE:
    {
      //To determine if comparison is allowed, lhs or rhs needs to be convertible to the type of the other
      //here, use the canConvert that takes an expression
      type = primitives[Prim::BOOL];
      if(!ltype->canConvert(rtype) && !rtype->canConvert(ltype))
      {
        errMsgLoc(this, ltype->getName() <<
            " and " << rtype->getName() << " can't be compared.");
      }
      if(ltype != rtype)
      {
        if(ltype->canConvert(rtype))
        {
          rhs = new Converted(rhs, ltype);
        }
        else
        {
          lhs = new Converted(lhs, rtype);
        }
      }
      break;
    }
    default: INTERNAL_ERROR;
  }
  resolved = true;
  cout << "Successfully resolved BinaryArith with type " << type->getName() << '\n';
}

/**********************
 * Primitive Literals *
 **********************/

IntLiteral::IntLiteral(IntLit* ast) : value(ast->val)
{
  setType();
  setLocation(ast);
  resolved = true;
}

IntLiteral::IntLiteral(uint64_t val) : value(val)
{
  setType();
  resolved = true;
}

void IntLiteral::setType()
{
  //use i32 if value fits, otherwise i64
  if(value > 0x7FFFFFFF)
  {
    type = primitives[Prim::LONG];
  }
  else
  {
    type = primitives[Prim::INT];
  }
}

FloatLiteral::FloatLiteral(FloatLit* a) : value(a->val)
{
  type = primitives[Prim::DOUBLE];
  setLocation(a);
  resolved = true;
}

FloatLiteral::FloatLiteral(double val) : value(val)
{
  type = primitives[Prim::DOUBLE];
  resolved = true;
}

StringLiteral::StringLiteral(StrLit* a)
{
  value = a->val;
  setLocation(a);
  type = getArrayType(primitives[Prim::CHAR], 1);
  resolved = true;
}

CharLiteral::CharLiteral(CharLit* ast)
{
  value = ast->val;
  setLocation(ast);
  type = primitives[Prim::CHAR];
  resolved = true;
}

BoolLiteral::BoolLiteral(bool v)
{
  value = v;
  type = primitives[Prim::BOOL];
  resolved = true;
}

/*******************
 * CompoundLiteral *
 *******************/

CompoundLiteral::CompoundLiteral(vector<Expression*>& mems)
  : members(mems) {}

void CompoundLiteral::resolveImpl(bool final)
{
  //first, try to resolve all members
  bool allResolved = true;
  lvalue = true;
  for(size_t i = 0; i < members.size(); i++)
  {
    resolveExpr(members[i], final);
    if(!members[i]->resolved)
    {
      allResolved = false;
      break;
    }
    if(!members[i]->assignable())
    {
      lvalue = false;
    }
  }
  if(!allResolved)
    return;
  vector<Type*> memberTypes;
  for(auto mem : members)
  {
    memberTypes.push_back(mem->type);
  }
  type = getTupleType(memberTypes);
  resolved = true;
}

/***********
 * Indexed *
 ***********/

Indexed::Indexed(Expression* grp, Expression* ind)
  : group(grp), index(ind) {}

void Indexed::resolveImpl(bool final)
{
  resolveExpr(group, final);
  resolveExpr(index, final);
  if(!group->resolved || !index->resolved)
  {
    return;
  }
  //Indexing a Tuple (literal, variable or call) requires the index to be an IntLit
  //Anything else is assumed to be an array and then the index can be any integer expression
  if(dynamic_cast<CompoundLiteral*>(group))
  {
    errMsgLoc(this, "Can't index a compound literal - assign it to an array first.");
  }
  //note: ok if this is null
  //in all other cases, group must have a type now
  if(auto tt = dynamic_cast<TupleType*>(group->type))
  {
    //group's type is a Tuple, whether group is a literal, var or call
    //make sure the index is an IntLit
    auto intIndex = dynamic_cast<IntLiteral*>(index);
    if(intIndex)
    {
      //int literals are always unsigned (in lexer) so always positive
      auto val = intIndex->value;
      if(val >= tt->members.size())
      {
        errMsgLoc(this, "tuple subscript out of bounds");
      }
    }
    else
    {
      errMsgLoc(this, "tuple subscript must be an integer constant.");
    }
  }
  else if(auto at = dynamic_cast<ArrayType*>(group->type))
  {
    //group must be an array
    type = at->subtype;
  }
  else if(auto mt = dynamic_cast<MapType*>(group->type))
  {
    //make sure ind can be converted to the key type
    if(!mt->key->canConvert(index->type))
    {
      errMsgLoc(this, "used incorrect type to index map");
    }
    //map lookup can fail, so return a "maybe" of value
    type = maybe(mt->value);
  }
  else
  {
    errMsgLoc(this, "expression can't be subscripted (is not an array, tuple or map)");
  }
  resolved = true;
}

/************
 * CallExpr *
 ************/

CallExpr::CallExpr(Expression* c, vector<Expression*>& a)
{
  auto ct = dynamic_cast<CallableType*>(c->type);
  if(!ct)
  {
    errMsg("expression is not callable");
  }
  callable = c;
  args = a;
}

void CallExpr::resolveImpl(bool final)
{
  resolveExpr(callable, final);
  if(!callable->resolved)
    return;
  auto callableType = dynamic_cast<CallableType*>(callable->type);
  if(!callableType)
  {
    errMsgLoc(this, "attempt to call non-callable expression");
  }
  type = callableType->returnType;
  bool allResolved = callable->resolved;
  for(size_t i = 0; i < args.size(); i++)
  {
    resolveExpr(args[i], final);
    allResolved = allResolved && args[i]->resolved;
  }
  if(!allResolved)
    return;
  //make sure number of arguments matches
  if(callableType->argTypes.size() != args.size())
  {
    errMsgLoc(this, "in call to " <<
        (callableType->ownerStruct ? "" : "static") <<
        (callableType->pure ? "function" : "procedure") <<
        ", expected " <<
        callableType->argTypes.size() <<
        " arguments but " <<
        args.size() << " were provided");
  }
  for(size_t i = 0; i < args.size(); i++)
  {
    //make sure arg value can be converted to expected type
    if(!callableType->argTypes[i]->canConvert(args[i]->type))
    {
      errMsg("argument " << i + 1 << " to " << (callableType->ownerStruct ? "" : "static") <<
        (callableType->pure ? "function" : "procedure") << " has wrong type (expected " <<
        callableType->argTypes[i]->getName() << " but got " <<
        (args[i]->type ? args[i]->type->getName() : "incompatible compound literal") << ")");
    }
    if(callableType->argTypes[i] != args[i]->type)
    {
      args[i] = new Converted(args[i], callableType->argTypes[i]);
    }
  }
  resolved = true;
}

/***********
 * VarExpr *
 ***********/

VarExpr::VarExpr(Variable* v, Scope* s) : var(v), scope(s) {}
VarExpr::VarExpr(Variable* v) : var(v), scope(nullptr) {}

void VarExpr::resolveImpl(bool final)
{
  cout << "Resolving variable for VarExpr.\n";
  var->resolveImpl(final);
  if(!var->resolved)
    return;
  cout << "  Resolution succeeded.\n";
  type = var->type;
  if(scope)
  {
    //only thing to check here is that var lives within
    //the innermost function containing the variable's usage
    //(or, if innermost function is a member, a member of that struct)
    Scope* varScope = var->scope;
    Scope* fnScope = scope->getFunctionContext();
    if(!fnScope->contains(varScope))
    {
      errMsgLoc(this, "use of variable " << var->name << " here violates function purity");
    }
  }
  resolved = true;
}

/******************
 * SubroutineExpr *
 ******************/

SubroutineExpr::SubroutineExpr(Subroutine* s)
{
  thisObject = nullptr;
  subr = s;
  exSubr = nullptr;
}

SubroutineExpr::SubroutineExpr(Expression* root, Subroutine* s)
{
  thisObject = root;
  subr = s;
  exSubr = nullptr;
}

SubroutineExpr::SubroutineExpr(ExternalSubroutine* es)
{
  thisObject = nullptr;
  subr = nullptr;
  exSubr = es;
}

void SubroutineExpr::resolveImpl(bool final)
{
  if(subr)
    type = subr->type;
  else if(exSubr)
    type = exSubr->type;
  else
    INTERNAL_ERROR;
  if(!thisObject && subr && subr->type->ownerStruct)
  {
    errMsgLoc(this, \
        "can't call member subroutine " << \
        subr->type->ownerStruct->name << '.' \
        << subr->name << \
        "\nwithout providing 'this' object");
  }
  else if(thisObject &&
      ((subr && !subr->type->ownerStruct) || exSubr))
  {
    errMsgLoc(this, \
        "can't call non-member subroutine " << \
        (subr ? subr->name : exSubr->name) << \
        " on an object");
  }
}

/*************
 * StructMem *
 *************/

StructMem::StructMem(Expression* b, Variable* v)
{
  base = b;
  member = v;
}

StructMem::StructMem(Expression* b, Subroutine* s)
{
  base = b;
  member = s;
}

void StructMem::resolveImpl(bool final)
{
  resolveExpr(base, final);
  if(!base->resolved)
  {
    return;
  }
  //make sure that member is actually a member of base
  if(member.is<Variable*>())
  {
    auto var = member.get<Variable*>();
    type = var->type;
    if(var->owner != base->type)
    {
      INTERNAL_ERROR;
    }
  }
  else
  {
    auto subr = member.get<Subroutine*>();
    type = subr->type;
    if(subr->type->ownerStruct != base->type)
    {
      INTERNAL_ERROR;
    }
  }
  resolved = true;
}

/************
 * NewArray *
 ************/

NewArray::NewArray(Type* elemType, vector<Expression*> dimensions)
{
  elem = elemType;
  dims = dimensions;
}

void NewArray::resolveImpl(bool final)
{
  resolveType(elem, final);
  if(!elem->resolved)
    return;
  for(size_t i = 0; i < dims.size(); i++)
  {
    resolveExpr(dims[i], final);
    if(!dims[i]->resolved)
    {
      return;
    }
    if(!dims[i]->type->isInteger())
    {
      errMsgLoc(dims[i], "array dimensions must be integers");
    }
  }
  type = getArrayType(elem, dims.size());
  resolved = true;
}

/***************
 * ArrayLength *
 ***************/

ArrayLength::ArrayLength(Expression* arr)
{
  array = arr;
}

void ArrayLength::resolveImpl(bool final)
{
  resolveExpr(array, final);
  if(!array->resolved)
  {
    return;
  }
  if(!array->type->isArray())
  {
    //len is not a keyword: <expr>.len is a special case
    //that should be handled in resolveExpr
    INTERNAL_ERROR;
  }
  type = primitives[Prim::UINT];
  resolved = true;
}

/************
 * ThisExpr *
 ************/

ThisExpr::ThisExpr(Scope* where)
{
  //figure out which struct "this" refers to,
  //or show error if there is none
  structType = where->getStructContext();
  if(!structType)
  {
    errMsgLoc(this, "can't use 'this' in static context");
  }
  type = structType;
  resolved = true;
}

/*************
 * Converted *
 *************/

Converted::Converted(Expression* val, Type* dst)
{
  if(!val->resolved)
  {
    INTERNAL_ERROR;
  }
  value = val;
  type = dst;
  resolved = true;
  //converted has same location as original value
  setLocation(val);
  if(!type->canConvert(value->type))
  {
    errMsgLoc(this, "can't implicitly convert from " << \
        val->type->getName() << " to " << type->getName());
  }
}

/************
 * EnumExpr *
 ************/

EnumExpr::EnumExpr(EnumConstant* ec)
{
  type = ec->et;
  value = ec->value;
  resolved = true;
}

/*********
 * Error *
 *********/

ErrorVal::ErrorVal()
{
  type = primitives[Prim::ERROR];
  resolved = true;
}

/*************************/
/* Expression resolution */
/*************************/

UnresolvedExpr::UnresolvedExpr(string n, Scope* s)
{
  base = nullptr;
  name = new Member;
  name->names.push_back(n);
  usage = s;
}

UnresolvedExpr::UnresolvedExpr(Member* n, Scope* s)
{
  base = nullptr;
  name = n;
  usage = s;
}

UnresolvedExpr::UnresolvedExpr(Expression* b, Member* n, Scope* s)
{
  base = b;
  name = n;
  usage = s;
}

void resolveExpr(Expression*& expr, bool final)
{
  if(expr->resolved)
  {
    return;
  }
  auto unres = dynamic_cast<UnresolvedExpr*>(expr);
  if(!unres)
  {
    expr->resolve(final);
    return;
  }
  Expression* base = unres->base; //might be null
  //set initial searchScope:
  //the struct scope if base is a struct, otherwise just usage
  size_t nameIter = 0;
  vector<string>& names = unres->name->names;
  cout << "Searching for expression name \"" << *(unres->name) << "\"\n";
  //first, get a base expression
  if(!base)
  {
    Scope* baseSearch = unres->usage;
    while(!base)
    {
      Name found = baseSearch->findName(names[nameIter]);
      if(!found.item)
      {
        if(!final)
        {
          //can't continue, but not an error either
          //(maybe the name just hasn't been declared yet)
          return;
        }
        string fullPath = names[0];
        for(int i = 0; i < nameIter; i++)
        {
          fullPath = fullPath + '.' + names[i];
        }
        errMsgLoc(unres, "unknown identifier " << fullPath);
      }
      //based on type of name, either set base or update search scope
      switch(found.kind)
      {
        case Name::MODULE:
          baseSearch = ((Module*) found.item)->scope;
          break;
        case Name::STRUCT:
          baseSearch = ((StructType*) found.item)->scope;
          break;
        case Name::SUBROUTINE:
          {
            auto subr = (Subroutine*) found.item;
            if(subr->type->ownerStruct)
            {
              //is a member subroutine, so create implicit "this"
              ThisExpr* subrThis = new ThisExpr(unres->usage);
              subrThis->resolve(true);
              //this must match owner type of subr
              if(subr->type->ownerStruct != subrThis->structType)
              {
                errMsgLoc(unres,
                    "implicit 'this' here can't be used to call " <<
                    subr->type->ownerStruct->name << '.' << subr->name);
              }
              base = new SubroutineExpr(subrThis, (Subroutine*) found.item);
            }
            else
            {
              //nonmember subroutine can be called from anywhere,
              //so no context checking here
              base = new SubroutineExpr(subr);
            }
            break;
          }
        case Name::EXTERN_SUBR:
          base = new SubroutineExpr((ExternalSubroutine*) found.item);
          break;
        case Name::VARIABLE:
          {
            auto var = (Variable*) found.item;
            if(var->owner)
            {
              ThisExpr* varThis = new ThisExpr(unres->usage);
              if(varThis->structType != var->owner)
              {
                errMsgLoc(unres,
                    "implicit 'this' here can't be used to access " <<
                    var->owner->name << '.' << var->name);
              }
              base = new StructMem(varThis, var);
            }
            else
            {
              cout << "Found variable with name " << var->name << ", creating VarExpr.\n";
              //static variable can be accessed anywhere
              base = new VarExpr(var);
            }
            break;
          }
        case Name::ENUM_CONSTANT:
          base = new EnumExpr((EnumConstant*) found.item);
          break;
        default:
          errMsgLoc(unres, "identifier is not a valid expression");
      }
      nameIter++;
    }
  }
  cout << "Resolving base expression.\n";
  base->resolve(final);
  //base must be resolved (need its type) to continue
  if(!base->resolved)
    return;
  //look up members in searchScope until a new expr can be formed
  while(nameIter < names.size())
  {
    if(base->type->isArray() && names[nameIter] == "len")
    {
      base = new ArrayLength(base);
      //this resolution can't fail
      base->resolve(true);
      nameIter++;
      continue;
    }
    auto baseStruct = dynamic_cast<StructType*>(base->type);
    if(!baseStruct)
    {
      errMsgLoc(unres, "cant access member of non-struct type " << base->type->getName());
    }
    bool validBase = false;
    Scope* baseSearch = baseStruct->scope;
    while(!validBase && nameIter < names.size())
    {
      Name found = baseSearch->findName(names[nameIter]);
      if(!found.item)
      {
        if(!final)
        {
          //can't continue, but not an error either
          return;
        }
        string fullPath = names[0];
        for(int i = 0; i < nameIter; i++)
        {
          fullPath = fullPath + '.' + names[i];
        }
        errMsgLoc(unres, "unknown identifier " << fullPath);
      }
      //based on type of name, either set base or update search scope
      switch(found.kind)
      {
        case Name::MODULE:
          baseSearch = ((Module*) found.item)->scope;
          break;
        case Name::SUBROUTINE:
          base = new SubroutineExpr((Subroutine*) found.item);
          validBase = true;
          break;
        case Name::VARIABLE:
          base = new StructMem(base, (Variable*) found.item);
          validBase = true;
          break;
        default:
          errMsgLoc(unres, "identifier " << names[nameIter] <<
              " is not a valid member of struct " << base->type->getName());
      }
    }
    if(!validBase)
    {
      //used up all the names but ended up with a module, not an expr
      string fullPath = names[0];
      for(int i = 0; i < nameIter; i++)
      {
        fullPath = fullPath + '.' + names[i];
      }
      errMsgLoc(unres, fullPath << " is not an expression");
    }
    base->resolve(final);
  }
  //save lexical location of original parsed expression
  base->setLocation(expr);
  expr = base;
}

