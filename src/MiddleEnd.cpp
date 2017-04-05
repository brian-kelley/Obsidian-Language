#include "MiddleEnd.hpp"

using namespace std;
using namespace Parser;

AP(Scope) MiddleEnd::loadScopes(AP(Parser::ModuleDef)& ast)
{
  AP(Scope) global(new BlockScope(NULL));
  loadBuiltinTypes(global);
  return global;
}

void MiddleEnd::loadBuiltinTypes(AP(Scope)& global)
{
  vector<Type>& table = global->types;
  table.push_back(new IntegerType("char", 1, true));
  table.push_back(new AliasType("i8", &table.back());
  table.push_back(new IntegerType("uchar", 1, false));
  table.push_back(new AliasType("u8", &table.back());
  table.push_back(new IntegerType("short", 2, true));
  table.push_back(new AliasType("i16", &table.back());
  table.push_back(new IntegerType("ushort", 2, false));
  table.push_back(new AliasType("u16", &table.back());
  table.push_back(new IntegerType("int", 4, true));
  table.push_back(new AliasType("i32", &table.back());
  table.push_back(new IntegerType("uint", 4, false));
  table.push_back(new AliasType("u32", &table.back());
  table.push_back(new IntegerType("long", 8, true));
  table.push_back(new AliasType("i64", &table.back());
  table.push_back(new IntegerType("ulong", 8, false));
  table.push_back(new AliasType("u64", &table.back());
  table.push_back(new FloatType("float", 4));
  table.push_back(new AliasType("f32", &table.back());
  table.push_back(new FloatType("double", 8));
  table.push_back(new AliasType("f64", &table.back());
  table.push_back(new StringType);
}

void MiddleEnd::semanticCheck(AP(Scope)& global)
{
}

void MiddleEnd::checkEntryPoint(AP(Scope)& global)
{
  bool found = false;
  for(auto& it : global->procs)
  {
    if(it->retType == 
  }

  ProcPrototype(Parser::ProcType& pt);
  bool nonterm;
  Type* retType;
  vector<Type*> argTypes;
}

void visitModule(Scope* current, AP(Parser::Module)& module);
void visitBlock(Scope* current, AP(Parser::Block)& module);
void visitStruct(Scope* current, AP(Parser::StructDecl)& module);

