#ifndef VARIABLE_H
#define VARIABLE_H

#include "Misc.hpp"
#include "Utils.hpp"
#include "TypeSystem.hpp"
#include "Scope.hpp"

struct Scope;

namespace TypeSystem
{
  struct Type;
}

struct Variable
{
  Scope* owner;
  string name;
  TypeSystem::Type* type;
};

#endif

