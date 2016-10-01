#include "Misc.hpp"
#include "Options.hpp"
#include "PreTokenize.hpp"

int main(int argc, const char** argv)
{
  Options op = parseOptions(argc, argv);
  if(argc == 1)
  {
    puts("Error: no input files.");
    return EXIT_FAILURE;
  }
  //Lex input file 
  return 0;
}
