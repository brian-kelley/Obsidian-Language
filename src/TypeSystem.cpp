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
  if(enclosing)
  {
    enclosing->types.push_back(this);
  }
}

void Type::createBuiltinTypes()
{
  primitives.push_back(new BoolType);

  primitives.emplace_back(new IntegerType("char", 1, true));
  new AliasType("i8", primitives.back(), global);

  primitives.emplace_back(new IntegerType("uchar", 1, false));
  new AliasType("u8", primitives.back(), global);

  primitives.emplace_back(new IntegerType("short", 2, true));
  new AliasType("i16", primitives.back(), global);

  primitives.emplace_back(new IntegerType("ushort", 2, false));
  new AliasType("u16", primitives.back(), global);

  primitives.emplace_back(new IntegerType("int", 4, true));
  new AliasType("i32", primitives.back(), global);

  primitives.emplace_back(new IntegerType("uint", 4, false));
  new AliasType("u32", primitives.back(), global);

  primitives.emplace_back(new IntegerType("long", 8, true));
  new AliasType("i64", primitives.back(), global);

  primitives.emplace_back(new IntegerType("ulong", 8, false));
  new AliasType("u64", primitives.back(), global);

  primitives.emplace_back(new FloatType("float", 4));
  new AliasType("f32", primitives.back(), global);

  primitives.emplace_back(new FloatType("double", 8));
  new AliasType("f64", primitives.back(), global);

  primitives.emplace_back(new StringType);
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
          arrays.push_back(new ArrayType(type, usedScope, dims));
          elemType->dimTypes.push_back(arrays.back());
        }
        //now return the needed type
        return elemType->dimTypes.back();
      }
    }
    else
    {
      //use undef type
      ArrayType* t = new ArrayType(nullptr, usedScope, dims);
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
  else if(type->t.is<AP(TupleTypeNT)>())
  {
    auto& tt = type->t.get<AP(TupleTypeNT)>();
    //search for each member individually
    vector<Type*> types;
    bool resolved = true;
    for(auto& it : tt->members)
    {
      types.push_back(getType(it.get(), usedScope));
      if(!types.back())
      {
        resolved = false;
      }
    }
    if(resolved)
    {
      //look up tuple by pointers, create if doesn't exist
      for(auto& existing : tuples)
      {
        if(existing->members.size() != types.size())
        {
          continue;
        }
        bool allMatch = true;
        for(size_t i = 0; i < types.size(); i++)
        {
          if(existing->members[i] != types[i])
          {
            allMatch = false;
            break;
          }
        }
        if(!allMatch)
        {
          continue;
        }
        //tuples equivalent, use existing
        return existing;
      }
    }
    //need to create new tuple (ctor adds to unresolvedTypes)
    return new TupleType(types);
  }
  else
  {
    //TODO: FuncPrototype, ProcPrototype
    INTERNAL_ERROR;
  }
  return NULL;
}

//resolve() called on type that doesn't implement it: error
void Type::resolve()
{
  INTERNAL_ERROR;
}

/***************/
/* Struct Type */
/***************/

StructType::StructType(Parser::StructDecl* sd, Scope* enclosingScope, StructScope* structScope) : Type(enclosingScope)
{
  this->name = sd->name;
  //can't actually handle any members yet - need to visit this struct decl as a scope first
  //but, this happens later
  decl = sd;
  //must assume there are unresolved members
  this->structScope = structScope;
  bool resolved = true;
  for(auto& it : sd->members)
  {
    ScopedDecl* decl = it->sd.get();
    if(decl->decl.is<AP(VarDecl)>())
    {
      VarDecl* data = decl->decl.get<AP(VarDecl)>().get();
      Type* dataType = getType(data->type.get(), structScope);
      if(!dataType)
      {
        resolved = false;
      }
      members.push_back(dataType);
      memberNames.push_back(data->name);
    }
  }
  if(!resolved)
  {
    unresolvedTypes.push_back(this);
  }
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

void StructType::resolve()
{
  //load all data member types, should be available now
  int memberNum = 0;
  for(auto& mem : decl->members)
  {
    auto& sd = mem->sd;
    if(sd->decl.is<AP(VarDecl)>())
    {
      //make sure this type was loaded correctly
      if(!members[memberNum])
      {
        Type* loaded = getType(sd->decl.get<AP(VarDecl)>()->type.get(), structScope);
        if(!loaded)
        {
          //TODO: decent error messages (Parser nonterms need to retain some token info (line/col))
          errAndQuit("Unknown type as struct member.");
        }
        members[memberNum] = loaded;
      }
      memberNum++;
    }
  }
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
  decl = ud;
}

void UnionType::resolve()
{
  //load all data member types, should be available now
  for(size_t i = 0; i < options.size(); i++)
  {
    if(!options[i])
    {
      Type* lookup = getType(decl->types[i].get(), enclosing);
      if(!lookup)
      {
        //TODO
        errAndQuit("Unknown type as union option.");
      }
      options[i] = lookup;
    }
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

ArrayType::ArrayType(Parser::TypeNT* type, Scope* enclosing, int dims) : Type(nullptr)
{
  //temporarily set dims to 0 while looking up element type
  type->arrayDims = 0;
  elem = getType(type, enclosing);
  type->arrayDims = dims;
  if(!elem)
  {
    unresolvedTypes.push_back(this);
  }
  this->dims = dims;
}

void ArrayType::resolve()
{
  if(!elem)
  {
    //temporarily set array type's AST node to 0 dimensions
    elemNT->arrayDims = 0;
    Type* lookup = getType(elemNT, enclosing);
    elemNT->arrayDims = dims;
    if(!lookup)
    {
      errAndQuit("Unknown type as array element.");
    }
  }
}

bool ArrayType::canConvert(Type* other)
{
  //TODO
  return false;
}

/**************/
/* Tuple Type */
/**************/

TupleType::TupleType(vector<Type*> members) : Type(nullptr)
{
  this->members = members;
  tuples.push_back(this);
  for(auto& it : members)
  {
    if(!it)
    {
      unresolvedTypes.push_back(this);
      return;
    }
  }
}

TupleType::TupleType(TupleTypeNT* tt, Scope* currentScope) : Type(nullptr)
{
  bool resolved = true;
  for(auto& it : tt->members)
  {
    TypeNT* typeNT = it.get();
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
  tuples.push_back(this);
}

void TupleType::resolve()
{
  for(size_t i = 0; i < members.size(); i++)
  {
    if(!members[i])
    {
      Type* lookup = getType(decl->members[i].get(), enclosing);
      if(!lookup)
      {
        errAndQuit("unknown type as member of tuple");
      }
      members[i] = lookup;
    }
  }
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

void AliasType::resolve()
{
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
  set<int> usedVals;
  vector<int> vals(e->items.size(), -1);
  //first, process all specified values
  for(size_t i = 0; i < e->items.size(); i++)
  {
    auto& item = *e->items[i];
    if(item.value)
    {
      vals[i] = item.value->val;
      if(usedVals.find(vals[i]) == usedVals.end())
      {
        usedVals.insert(vals[i]);
      }
      else
      {
        string errMsg = "Enum \"";
        errMsg += e->name + "\" has a duplicate value " + to_string(vals[i]) + " with key \"" + item.name + "\"";
        errAndQuit(errMsg);
      }
    }
  }
  //now fill in remaining values automatically (start at 0)
  int autoVal = 0;
  for(size_t i = 0; i < e->items.size(); i++)
  {
    if(vals[i] < 0)
    {
      //need a value for this key, pick one that hasn't been used already
      while(usedVals.find(autoVal) != usedVals.end())
      {
        autoVal++;
      }
      vals[i] = autoVal;
      usedVals.insert(autoVal);
    }
  }
  for(size_t i = 0; i < e->items.size(); i++)
  {
    values[e->items[i]->name] = vals[i];
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

IntegerType::IntegerType(string name, int size, bool sign) : Type(nullptr)
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

FloatType::FloatType(string name, int size) : Type(nullptr)
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

StringType::StringType() : Type(nullptr) {}

bool StringType::canConvert(Type* other)
{
  //TODO
  return false;
}

/*************/
/* Bool Type */
/*************/

BoolType::BoolType() : Type(nullptr) {}

bool BoolType::canConvert(Type* other)
{
  //TODO
  return false;
}


