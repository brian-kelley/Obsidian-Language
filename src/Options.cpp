#include "Options.hpp" 

Options getDefaultOptions()
{
  Options op;
  op.emitC = false;
  op.input = "";
  op.outputStem = "";
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
        op.outputStem = argv[++i];
        if(op.outputStem.find(".exe") != string::npos)
          op.outputStem = op.outputStem.substr(0, op.outputStem.length() - 4);
      }
      else
      {
        puts("Error: output file not specified.");
        exit(EXIT_FAILURE);
      }
    }
    else if(strcmp(argv[i], "--c") == 0)
    {
      op.emitC = true;
    }
    else
    {
      op.input = argv[i];
    }
  }
  if(op.input == "")
  {
    puts("Error: no input files.");
    exit(EXIT_FAILURE);
  }
  else if(op.input.length() < 3 || op.input.substr(op.input.length() - 3, 3) != ".os")
  {
    puts("Error: input file does not have .os file extension.");
    exit(EXIT_FAILURE);
  }
  if(op.outputStem == "")
  {
    size_t stemStart = 0;
    auto find = op.input.rfind("/");
    if(find != string::npos)
      stemStart = find + 1;
    op.outputStem = op.input.substr(stemStart, op.input.length() - stemStart - 3);
  }
  return op;
}

