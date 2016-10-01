#include "Options.hpp"

Options getDefaultOptions()
{
  Options op;
  op.emitPreprocess = false;
  op.emitC = false;
  op.input = "";
  op.output = "";
  return op;
}

Options parseOptions(int argc, const char** argv)
{
  if(argc == 1)
  {
    puts("Error: no input files.");
    exit(EXIT_FAILURE);
  }
  Options op = getDefaultOptions();
  for(int i = 1; i < argc; i++)
  {
    if(strcmp(argv[i], "--output") == 0)
    {
      if(i < argc - 1)
      {
        op.output = argv[++i];
      }
      else
      {
        puts("Error: output file not specified.");
        exit(EXIT_FAILURE);
      }
    }
    else if(strcmp(argv[i], "--preprocess") == 0)
      op.emitPreprocess = true;
    else if(strcmp(argv[i], "--c") == 0)
      op.emitC = true;
    else
      op.input = argv[i];
  }
  if(op.input == "")
  {
    puts("Error: no input files.");
    exit(EXIT_FAILURE);
  }
  return op;
}

