#include "Common.hpp"
#include "Subroutine.hpp"
#include "Variable.hpp"
#include "Module.hpp"

extern Module* global;

struct CFG
{
  Subroutine* subr;
  void build();
};

