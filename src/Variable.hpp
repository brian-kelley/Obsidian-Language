#ifndef VARIABLE_H
#define VARIABLE_H

#include "Misc.hpp"
#include "Utils.hpp"
#include "TypeSystem.hpp"
#include "Scope.hpp"

struct Scope;
struct Type;

struct Variable
{
  Scope* owner;
  string name;
  Type* type;
};

#endif

