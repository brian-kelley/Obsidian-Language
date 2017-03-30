#include "Misc.hpp"
#include "Options.hpp"
#include "Utils.hpp"
#include "Preprocess.hpp"
#include "CGen.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

void init()
{
  //all namespace initialization
  initTokens();
}

int main(int argc, const char** argv)
{
  init();
  Options op = parseOptions(argc, argv);
  if(argc == 1)
  {
    puts("Error: no input files.");
    return EXIT_FAILURE;
  }
  string code = loadFile(op.input.c_str());
  cout << "Loaded " << code.size() << " bytes of source code.\n";
  cout << "Will compile executable \"" << op.outputStem + ".exe" << "\"\n";
  /*
  if(op.emitPreprocess)
  {
    cout << "Will emit preprocessed code to \"" << op.outputStem + ".obp" << "\"\n";
  }
  */
  if(op.emitC)
  {
    cout << "Will emit C++ code to \"" << op.outputStem + ".c" << "\"\n";
  }
  //Preprocessing
  preprocess(code);
  //Lexing
  vector<Token*> toks = lex(code);
  //Parsing
  UP<Parser::ModuleDef> ast = Parser::parseProgram(toks);
  //Code generation
  if(op.emitC)
  {
    generateC(op.outputStem, op.emitC, ast);
  }
  return 0;
}

