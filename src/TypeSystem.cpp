#include "TypeSystem.hpp"

using namespace std;
using namespace Parser;

/***********************/
/* Type and subclasses */
/***********************/

struct ModuleScope;

extern ModuleScope* global;

vector<Type*> Type::primitives;
vector<TupleType*> Type::tuples;
vector<ArrayType*> Type::arrays;
vector<Type*> Type::unresolvedTypes;

Type::Type(Scope* enclosingScope)
{
  enclosing = enclosingScope;
  enclosingScope->types.push_back(this);
}

void Type::createBuiltinTypes()
{
#define ADD_PRIM primitives.push_back(table.back())
  vector<Type*>& table = global->types;
  table.emplace_back(new BoolType);
  ADD_PRIM;
  table.emplace_back(new IntegerType("char", 1, true));
  ADD_PRIM;
  table.emplace_back(new AliasType("i8", table.back(), global));
  table.emplace_back(new IntegerType("uchar", 1, false));
  ADD_PRIM;
  table.emplace_back(new AliasType("u8", table.back(), global));
  table.emplace_back(new IntegerType("short", 2, true));
  ADD_PRIM;
  table.emplace_back(new AliasType("i16", table.back(), global));
  table.emplace_back(new IntegerType("ushort", 2, false));
  ADD_PRIM;
  table.emplace_back(new AliasType("u16", table.back(), global));
  table.emplace_back(new IntegerType("int", 4, true));
  ADD_PRIM;
  table.emplace_back(new AliasType("i32", table.back(), global));
  table.emplace_back(new IntegerType("uint", 4, false));
  ADD_PRIM;
  table.emplace_back(new AliasType("u32", table.back(), global));
  table.emplace_back(new IntegerType("long", 8, true));
  ADD_PRIM;
  table.emplace_back(new AliasType("i64", table.back(), global));
  table.emplace_back(new IntegerType("ulong", 8, false));
  ADD_PRIM;
  table.emplace_back(new AliasType("u64", table.back(), global));
  table.emplace_back(new FloatType("float", 4));
  ADD_PRIM;
  table.emplace_back(new AliasType("f32", table.back(), global));
  table.emplace_back(new FloatType("double", 8));
  ADD_PRIM;
  table.emplace_back(new AliasType("f64", table.back(), global));
  table.emplace_back(new StringType);
  ADD_PRIM;
#undef ADD_PRIM
}

Type* Type::getType(Parser::TypeNT* type, Scope* usedScope)
{
  //handle array immediately - just make an array and then handle singular type
  if(type->arrayDims)
  {
    size_t dims = type->arrayDims;
    type->arrayDims = 0;
    //now look up the type for the element type
    Type* elemType = getType(type, usedScope);
    //restore original type to preserve AST
    type->arrayDims = dims;
    if(elemType)
    {
      //lazily check & create array type
      if(elemType->dimTypes.size() >= dims)
      {
        //already exists
        return elemType->dimTypes[dims - 1];
      }
      else
      {
        //create + add
        //size = 1 -> max dim = 1
        for(size_t i = elemType->dimTypes.size(); i <= dims; i++)
        {
          arrays.push_back(new ArrayType(elemType, dims));
          elemType->dimTypes.push_back(arrays.back());
        }
        //now return the needed type
        return elemType->dimTypes.back();
      }
    }
    else
    {
      //use undef type
      ArrayType* t = new ArrayType(nullptr, dims);
      arrays.push_back(t);
      unresolvedTypes.push_back(t);
      return t;
    }
  }
  if(type->t.is<TypeNT::Prim>())
  {
    return primitives[(int) type->t.get<TypeNT::Prim>()];
  }
  else if(type->t.is<AP(Member)>())
  {
    //search up scope tree for the member
    //need to search for EnumType, AliasType, StructType or UnionType
    for(Scope* iter = usedScope; iter; iter = iter->parent)
    {
      Scope* memScope = iter;
      //iter is the root of search (scan for child scopes, then the type)
      for(Member* search = type->t.get<AP(Member)>().get();
          search; search = search->mem.get())
      {
        bool foundNext = false;
        if(search->mem)
        {
          //find scope with name mem->owner
          for(auto& searchScope : memScope->children)
          {
            if(searchScope->getLocalName() == search->owner)
            {
              //found the next memScope for searching
              foundNext = true;
              memScope = searchScope;
              break;
            }
          }
          if(!foundNext)
          {
            //stop searching down this chain of scopes
            break;
          }
        }
        else
        {
          //find type with name mem->owner
          for(auto& searchType : iter->types)
          {
            StructType* st = dynamic_cast<StructType*>(searchType);
            if(st && st->name == search->owner)
            {
              return st;
            }
            UnionType* ut = dynamic_cast<UnionType*>(searchType);
            if(ut && ut->name == search->owner)
            {
              return ut;
            }
            EnumType* et = dynamic_cast<EnumType*>(searchType);
            if(et && et->name == search->owner)
            {
              return et;
            }
            AliasType* at = dynamic_cast<AliasType*>(searchType);
            if(at && at->name == search->owner)
            {
              return at;
            }
          }
        }
      }
    }
  }
  else
  {
    //TODO: FuncPrototype, ProcPrototype
    INTERNAL_ERROR;
  }
  return NULL;
}

/***************/
/* Struct Type */
/***************/

StructType::StructType(string name, Scope* enclosingScope) : Type(enclosingScope)
{
  this->name = name;
  decl = nullptr;
  unresolvedTypes.push_back(this);
}

StructType::StructType(Parser::StructDecl* sd, Scope* enclosingScope) : Type(enclosingScope)
{
  this->name = sd->name;
  //can't actually handle any members yet - need to visit this struct decl as a scope first
  //but, this happens later
  decl = sd;
  //must assume there are unresolved members
  unresolvedTypes.push_back(this);
}

bool StructType::hasFunc(FuncPrototype* type)
{
  //TODO
  return false;
}

bool StructType::hasProc(ProcPrototype* type)
{
  //TODO
  return false;
}

bool StructType::canConvert(Type* other)
{
  //TODO
  return false;
}

/**************/
/* Union Type */
/**************/

UnionType::UnionType(Parser::UnionDecl* ud, Scope* enclosingScope) : Type(enclosingScope)
{
  bool resolved = true;
  this->name = ud->name;
  for(auto& it : ud->types)
  {
    Type* option = getType(it.get(), enclosingScope);
    if(!option)
    {
      resolved = false;
      options.push_back(option);
    }
  }
  if(!resolved)
  {
    unresolvedTypes.push_back(this);
  }
}

bool UnionType::canConvert(Type* other)
{
  //TODO
  return false;
}

/**************/
/* Array Type */
/**************/

ArrayType::ArrayType(Type* elemType, int dims) : Type(global)
{
  this->elem = elemType;
  this->dims = dims;
}

bool ArrayType::canConvert(Type* other)
{
  //TODO
  return false;
}

/**************/
/* Tuple Type */
/**************/

TupleType::TupleType(TupleTypeNT* tt, Scope* currentScope) : Type(global)
{
  bool resolved = true;
  for(size_t i = 0; i < tt->members.size(); i++)
  {
    TypeNT* typeNT = tt->members[i].get();
    Type* type = getType(typeNT, currentScope);
    if(!type)
    {
      resolved = false;
    }
    members.push_back(type);
  }
  if(!resolved)
  {
    unresolvedTypes.push_back(this);
    //will visit this later and look up all NULL types again
  }
  decl = tt;
}

bool TupleType::canConvert(Type* other)
{
  //TODO
  return false;
}

/**************/
/* Alias Type */
/**************/

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

AliasType::AliasType(string alias, Type* underlying, Scope* currentScope) : Type(currentScope)
{
  name = alias;
  actual = underlying;
  decl = nullptr;
}

bool AliasType::canConvert(Type* other)
{
  return actual->canConvert(other);
}

/*************/
/* Enum Type */
/*************/

EnumType::EnumType(Parser::Enum* e, Scope* current) : Type(current)
{
  name = e->name;
  for(auto& it : e->items)
  {
    values[it->name] = it->value->val;
  }
}

bool EnumType::canConvert(Type* other)
{
  //TODO
  return false;
}

/****************/
/* Integer Type */
/****************/

IntegerType::IntegerType(string name, int size, bool sign) : Type(global)
{
  this->name = name;
  this->size = size;
  this->isSigned = sign;
}

bool IntegerType::canConvert(Type* other)
{
  //TODO
  return false;
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

/**************/
/* Float Type */
/**************/

FloatType::FloatType(string name, int size) : Type(global)
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

bool FloatType::canConvert(Type* other)
{
  //TODO
  return false;
}

/***************/
/* String Type */
/***************/

StringType::StringType() : Type(global) {}

bool StringType::canConvert(Type* other)
{
  //TODO
  return false;
}

/*************/
/* Bool Type */
/*************/

BoolType::BoolType() : Type(global) {}

bool BoolType::canConvert(Type* other)
{
  //TODO
  return false;
}


