#ifndef OPTIONS_H
#define OPTIONS_H

#include "Misc.hpp"

struct Options
{
  string input;
  string outputStem;
  bool emitPreprocess;
  bool emitCPP;
};

Options getDefaultOptions();
Options parseOptions(int argc, const char** argv);

#endif

