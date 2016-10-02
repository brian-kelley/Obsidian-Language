#include "Misc.hpp"
#include "Options.hpp"
#include "Utils.hpp"
#include "Preprocess.hpp"
#include "CppGen.hpp"

int main(int argc, const char** argv)
{
  Options op = parseOptions(argc, argv);
  if(argc == 1)
  {
    puts("Error: no input files.");
    return EXIT_FAILURE;
  }
  string code = loadFile(op.input.c_str());
  cout << "Loaded " << code.size() << " bytes of source code.\n";
  cout << "Will compile executable \"" << op.outputStem + ".exe" << "\"\n";
  if(op.emitPreprocess)
  {
    cout << "Will emit preprocessed code to \"" << op.outputStem + ".obp" << "\"\n";
  }
  if(op.emitCPP)
  {
    cout << "Will emit C++ code to \"" << op.outputStem + ".cpp" << "\"\n";
  }
  //Preprocess
  preprocess(code);
  if(op.emitPreprocess)
  {
    writeFile(code, op.outputStem + ".obp");
  }
  //??? Compile ???
  if(op.emitCPP)
  {
    generateCPP(op.outputStem, op.emitCPP, code);
  }
  return 0;
}

