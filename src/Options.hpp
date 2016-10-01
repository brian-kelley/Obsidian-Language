#ifndef OPTIONS_H
#define OPTIONS_H

#include "Misc.hpp"

struct Options
{
  string input;
  string output;
  bool emitPreprocess;
  bool emitC;
}

Options getDefaultOptions();
Options parseOptions(int argc, const char** argv);

#endif
