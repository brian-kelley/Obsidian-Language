#include "Options.hpp" 
#include <cstring>

Options getDefaultOptions()
{
  Options op;
  op.emitC = false;
  op.input = "";
  op.output = "";
  op.verbose = false;
  op.interactive = false;
  return op;
}

Options parseOptions(int argc, const char** argv)
{
  if(argc == 1)
  {
    puts("Error: no input.");
    exit(EXIT_FAILURE);
  }
  Options op = getDefaultOptions();
  for(int a = 1; a < argc; a++)
  {
    if(op.interactive || op.input.length())
    {
      //Have input file (or interactive flag),
      //all remaining args will be passed to main()
      //by interpreter
      op.interpArgs.emplace_back(argv[a]);
      cout << "Got interp arg: \"" << op.interpArgs.back() << "\"\n";
      continue;
    }
    if(!strcmp(argv[a], "-i"))
      op.interactive = true;
    else if(!strcmp(argv[a], "-a"))
      op.emitC = true;
    else if(!strcmp(argv[a], "-v"))
      op.verbose = true;
    else if(!strcmp(argv[a], "-o"))
    {
      op.output = argv[++a];
    }
    else
    {
      if(op.input.length())
        errMsg("Must provide a single main input file");
      op.input = argv[a];
    }
  }
  return op;
}

