#include "TypeSystem.hpp"

using namespace std;
using namespace Parser;

/***********************/
/* Type and subclasses */
/***********************/

extern AP(ModuleScope) global;

vector<TupleType*> Type::tuples;
map<string, Type*> Type::primitives;
vector<Type*> Type::unresolvedTypes;

Type::Type(Scope* enclosingScope)
{
  enclosing = enclosingScope;
  enclosingScope->types.push_back(this);
}

void Type::createBuiltinTypes(Scope* global)
{
#define ADD_PRIM primitives.push_back(&table.back().get())
  vector<AP(Type)>& table = global->types;
  table.emplace_back(new BoolType);
  ADD_PRIM;
  table.emplace_back(new IntegerType("char", 1, true));
  ADD_PRIM;
  table.emplace_back(new AliasType("i8", &table.back()));
  table.emplace_back(new IntegerType("uchar", 1, false));
  ADD_PRIM;
  table.emplace_back(new AliasType("u8", &table.back()));
  table.emplace_back(new IntegerType("short", 2, true));
  ADD_PRIM;
  table.emplace_back(new AliasType("i16", &table.back()));
  table.emplace_back(new IntegerType("ushort", 2, false));
  ADD_PRIM;
  table.emplace_back(new AliasType("u16", &table.back()));
  table.emplace_back(new IntegerType("int", 4, true));
  ADD_PRIM;
  table.emplace_back(new AliasType("i32", &table.back()));
  table.emplace_back(new IntegerType("uint", 4, false));
  ADD_PRIM;
  table.emplace_back(new AliasType("u32", &table.back()));
  table.emplace_back(new IntegerType("long", 8, true));
  ADD_PRIM;
  table.emplace_back(new AliasType("i64", &table.back()));
  table.emplace_back(new IntegerType("ulong", 8, false));
  ADD_PRIM;
  table.emplace_back(new AliasType("u64", &table.back()));
  table.emplace_back(new FloatType("float", 4));
  ADD_PRIM;
  table.emplace_back(new AliasType("f32", &table.back()));
  table.emplace_back(new FloatType("double", 8));
  ADD_PRIM;
  table.emplace_back(new AliasType("f64", &table.back()));
  table.emplace_back(new StringType);
  ADD_PRIM;
#undef ADD_PRIM
}

Type* Type::getType(Parser::TypeNT* type, Scope* usedScope)
{
  if(type->t.is<TypeNT::Prim>())
  {
  }
        /*
    TypeNT();
    Type* entry;        //TypeSystem type table entry for this
    enum struct Prim
    {
      BOOL,
      CHAR,
      UCHAR,
      SHORT,
      USHORT,
      INT,
      UINT,
      LONG,
      ULONG,
      FLOAT,
      DOUBLE,
      STRING
    };
    variant<
      None,
      Prim,
      AP(Member),
      AP(TupleTypeNT),
      AP(FuncType),
      AP(ProcType)> t;
    int arrayDims;
    */
}

//Get the type table entry, given the local usage name and current scope
//If type not defined, return NULL
Type* Type::getType(string localName, Scope* usedScope)
{
  Parser::Member mem;
  mem.owner = localName;
  mem.member = AP(Member)(nullptr);
  return getType(&mem, usedScope, true);
}

Type* Type::getType(Parser::Member* localName, Scope* usedScope, bool searchUp)
{
  if(localName->member.get())
  {
    //get scope with name localName->owner, then try there with tail of localName
    for(auto& it : usedScope->children)
    {
      if(it->getLocalName() == localName->owner)
      {
        //can only be one child scope with that name
        //only need to search this one scope (it)
        return getType(localName->member, it, false);
      }
    }
  }
  else
  {
    //localName is just the type name, search for it in this scope
    for(auto& it : usedScope->types)
    {
      if(it->getLocalName() == localName->owner)
      {
        return it;
      }
    }
  }
  if(searchUp && usedScope->parent)
  {
    return getType(localName, usedScope->parent, false);
  }
  else
  {
    return NULL;
  }
}

StructType(Parser::StructDecl* sd, Scope* enclosingScope) : Type(enclosingScope)
{
  this->name = sd->name;
  //can't actually handle any members yet - need to visit this struct decl as a scope first
  //but, this happens later
  decl = sd;
  //must assume there are unresolved members
  unresolvedTypes.push_back(this);
}

bool StructType::hasFunc(FuncType* type)
{
}

bool StructType::hasProc(ProcType* type)
{
}

TupleType::TupleType(TupleTypeNT* tt, Scope* currentScope) : Type(currentScope)
{
  bool unresolved = false;
  for(size_t i = 0; i < tt->members.size(); i++)
  {
    TypeNT* typeNT = tt->members[i];
    Type* type = getType(typeNT, currentScope);
    if(!type)
    {
      unresolved = true;
    }
    members.push_back(type);
  }
  if(unresolved)
  {
    unresolvedTypes.push_back(this);
    //will visit this later and look up all NULL types again
  }
  decl = tt;
}

Type* Type::getTypeOrUndef(TypeNT* nt, Scope* currentScope, Type* usage, int tupleIndex)
{
  Type* lookup = getType(td->type, currentScope);
  if(!lookup)
  {
    //UndefType(string name, Scope* enclosing, Type* usageType) : Type(enclosing)
    //Failed lookup: underlying parsed type must be a Member
    //Must use member's full name for unambiguous aliased type
    return new UndefType(nt->t.get<AP(Member)>().get(), currentScope, usage, tupleIndex);
  }
  else
  {
    //lookup successful so use known underlying type
    return lookup;
  }
}

AliasType::AliasType(Typedef* td, Scope* current) : Type(current)
{
  name = td->ident;
  Type* t = getType(td->type.get(), current);
  actual = t;
  if(!t)
  {
    unresolvedTypes.push_back(this);
  }
  decl = td;
}

EnumType::EnumType(Parser::Enum* e, Scope* current) : Type(current)
{
  name = e->name;
  for(auto& it : e->items)
  {
    values[it->name] = it->value->val;
  }
}

IntegerType::IntegerType(string name, int size, bool sign)
{
  this->name = name;
  this->size = size;
  this->isSigned = sign;
}

/*
string IntegerType::getCName()
{
  if(size == 1 && isSigned)
    return "char";
  else if(size == 1)
    return "uchar";
  else if(size == 2 && isSigned)
    return "short";
  else if(size == 2)
    return "ushort";
  else if(size == 4 && isSigned)
    return "int";
  else if(size == 4)
    return "uint";
  else if(size == 8 && isSigned)
    return "long";
  else if(size == 8)
    return "ulong";
  else
  {
    //todo: support arbitrary-sized ints as builtin types
  }
}
*/
FloatType::FloatType(string name, int size)
{
  this->name = name;
  this->size = size;
}

/*
string FloatType::getCName()
{
  if(size == 4)
  {
    return "float";
  }
  else if(size == 8)
  {
    return "double"
  }
}
*/

