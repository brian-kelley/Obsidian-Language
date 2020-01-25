#ifndef OPTIONS_H
#define OPTIONS_H

#include "Common.hpp"
#include "string.h"

struct Options
{
  string input;
  string output;
  int backend;
  //emit intermediate c source
  bool emitC;
  bool verbose;
  bool interactive;
  vector<string> interpArgs;
};

Options getDefaultOptions();
Options parseOptions(int argc, const char** argv);

#endif

