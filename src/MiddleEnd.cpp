#include "MiddleEnd.hpp"

namespace MiddleEnd
{
  void MiddleEnd::load(AP(Module)& ast)
  {
    AP(Scope) global(new ModuleScope(NULL));
    ast->scope = global.get();
    loadBuiltinTypes(global);
    //build scope tree
    visitModule(NULL, ast);
  }

  void MiddleEnd::loadBuiltinTypes(AP(Scope)& global)
  {
    vector<Type*>& table = global->types;
    table.push_back(new IntegerType("char", 1, true));
    table.push_back(new AliasType("i8", &table.back()));
    table.push_back(new IntegerType("uchar", 1, false));
    table.push_back(new AliasType("u8", &table.back()));
    table.push_back(new IntegerType("short", 2, true));
    table.push_back(new AliasType("i16", &table.back()));
    table.push_back(new IntegerType("ushort", 2, false));
    table.push_back(new AliasType("u16", &table.back()));
    table.push_back(new IntegerType("int", 4, true));
    table.push_back(new AliasType("i32", &table.back()));
    table.push_back(new IntegerType("uint", 4, false));
    table.push_back(new AliasType("u32", &table.back()));
    table.push_back(new IntegerType("long", 8, true));
    table.push_back(new AliasType("i64", &table.back()));
    table.push_back(new IntegerType("ulong", 8, false));
    table.push_back(new AliasType("u64", &table.back()));
    table.push_back(new FloatType("float", 4));
    table.push_back(new AliasType("f32", &table.back()));
    table.push_back(new FloatType("double", 8));
    table.push_back(new AliasType("f64", &table.back()));
    table.push_back(new StringType);
  }

  void MiddleEnd::semanticCheck(AP(Scope)& global)
  {
  }

  void MiddleEnd::checkEntryPoint(AP(Scope)& global)
  {
  }

  namespace TypeLoading
  {
    void visitModule(Scope* current, AP(Module)& m)
    {
      AP(Scope) mscope(new ModuleScope);
      if(current)
      {
        current->children.push_back(mscope);
      }
      mscope->name = m->name;
      m->scope = mscope.get();
      //add all locally defined, non-scope types in first pass:
      //typedefs, enums, variants
      for(auto& it : m->decls)
      {
        if(it->decl.is<AP(Typedef)>() ||
            it->decl.is<AP(Enum)>() ||
            it->decl.is<AP(VariantDecl)>())
        {
          visitScopedDecl(mscope.get(), it);
        }
      }
    }

    void visitBlock(Scope* current, AP(Block)& b)
    {
      AP(Scope) bscope(new BlockScope);
      current->children.push_back(bscope);
      bscope->index = BlockScope::nextBlockIndex++;
      b->scope = bscope.get();
      for(auto& st : b->statements)
      {
        if(st->s.is<AP(ScopedDecl)>())
        {
          visitScopedDecl(bscope.get(), st->s.get<AP(ScopedDecl)>());
        }
      }
    }

    void visitStruct(Scope* current, AP(StructDecl)& sd)
    {
      //must create a child scope and also a type
      AP(Scope) sscope(new StructScope);
      current->children.push_back(sscope);
      sscope->name = sd->name;
      sd->scope = sscope.get();
      //create the type, but don't deal with traits and
      //  members yet
      AP(StructType) stype(new StructType(sd->name, current));
      for(auto& it : sd->members)
      {
        auto& decl = it->sd;
        //deal with typedefs, using, other structs, enums etc. inside struct
        visitScopedDecl(sscope.get(), decl);
      }
    }

    void visitScopedDecl(Scope* current, AP(ScopedDecl)& sd)
    {
    }
  }
}

