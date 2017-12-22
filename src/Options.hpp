#ifndef OPTIONS_H
#define OPTIONS_H

#include "Common.hpp"
#include "string.h"

struct Options
{
  string input;
  string outputStem;
  int backend;
  //emit intermediate c source
  bool emitC;
};

Options getDefaultOptions();
Options parseOptions(int argc, const char** argv);

#endif

