#ifndef VARIABLE_H
#define VARIABLE_H

#include "Misc.hpp"
#include "Utils.hpp"
#include "Type.hpp"

struct Variable
{
  Scope* owner;
  string name;
  Type* type;
};

#endif

