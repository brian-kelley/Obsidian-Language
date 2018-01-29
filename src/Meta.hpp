#ifndef META_H
#define META_H

#include "Common.hpp"
#include "Parser.hpp"

//Type system and interpreter for Onyx meta-language

/*
  -need to have all parser symbols as data types
  -emit keyword: emit(thing) inserts thing into normal program text
    -thing can be a string (just inserts text verbatim)
    -or thing can be a real parse tree node
    -is possible for thing to contain meta-stuff, so meta stuff can be recursive
  -implicit conversion from string to any terminals/nonterminal allowed
    -this parses the thing from the text, is compiler error if that fails
  -meta-declarations only live in meta space, so there can be no name conflicts between 
    -if e.g. a struct is desired in both spaces,
    have to do that manually, but it's easy:
      #proc void bothSpaces(Decl d)
      {
        emit('#');
        emit(d);
        emit(d);
      }
      //then can do:
      bothSpaces(
        struct Thing
        {
          string name;
          int[] blah;
        };
      );
  -everything in meta-space has the same grammar as regular language,
    but this file handles interpretation of regular language and replacing #
    things with proper things
 */

namespace Meta
{
  //Interp: handle some parsed element after '#'
  //(there are only a few basic things that need to go here)
  namespace Interp
  {
    struct Value
    {
      virtual ~Value();
      Type* type;
    };

    struct PrimitiveValue : public Value
    {
      PrimitiveValue() : type(nullptr) {}
      PrimitiveValue(PrimitiveValue& pv)
      {
        type = pv.type;
        data = pv.data;
        size = pv.size;
        t = pv.t;
      }
      union
      {
        long long ll;
        unsigned long long ull;
        double d;
        float f;
        bool b;
      } data;
      enum Type
      {
        LL,
        ULL,
        D,
        F,
        B
      };
      Type t;
      //size, in bytes
      //only matters if t is LL or ULL
      int size;
    };

    //CompoundValue covers both structs and tuples
    struct CompoundValue : public Value
    {
      vector<Value*> members;
    };

    struct UnionValue : public Value
    {
      //which option is actually contained in v
      Type* actual;
      Value* v;
    };

    struct ArrayValue : public Value
    {
      vector<Value*> data;
    };

    struct MapValue : public Value
    {
      map<Value*, Value*> data;
    };

    Value* expression(Expression* expr);
    void statement(Statement* stmt);
  }
}

#endif

