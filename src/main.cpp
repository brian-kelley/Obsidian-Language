#include "Common.hpp"
#include "Options.hpp"
//#include "C_Backend.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "AST.hpp"
#include "AST_Output.hpp"
#include "IR.hpp"
#include "Dataflow.hpp"
#include "IRDebug.hpp"
#include "BuiltIn.hpp"

Module* global = nullptr;

void init()
{
  //all namespace initialization
  initTokens();
  global = new Module("", nullptr);
  createBuiltinTypes();
  //C::init();
}

void semanticCheck()
{
  global->resolve();
  if(!programHasMain)
  {
    errMsg("Program requires main procedure to be defined");
  }
}

int main(int argc, const char** argv)
{
  auto startTime = clock();
  Options op = parseOptions(argc, argv);
  if(argc == 1)
  {
    puts("Error: no input files.");
    return EXIT_FAILURE;
  }
  //init creates the builtin declarations
  init();
  //all program code: prepend builtin code to source file
  //string code = getBuiltins() + loadFile(op.input);
  //DEBUG_DO(cout << "Compiling " << code.size() << " bytes of source code, including builtins\n";);
  //Lexing
  //Parse the global/root module
  TIMEIT("Parsing", parseProgram(op.input););
  //DEBUG_DO(outputAST(global, "parse.dot"););
  TIMEIT("Semantic analysis", global->resolve(););
  DEBUG_DO(outputAST(global, "AST.dot");)
  TIMEIT("IR/CFG construction", IR::buildIR();)
  //DEBUG_DO(IRDebug::dumpIR("ir.dot");)
  TIMEIT("Optimizing IR", IR::optimizeIR();)
  //TIMEIT("C generate & compile", C::generate(op.outputStem, true););
  //Code generation
  auto elapsed = (double) (clock() - startTime) / CLOCKS_PER_SEC;
  cout << "Compilation completed in " << elapsed << " seconds.\n";
  //int lines = PastEOF::inst.line;
  /*
  cout << "Processed " << lines << " lines (" << lines / elapsed;
  cout << " lines/s, or " << code.length() / (elapsed * 1000000) << " MB/s)\n";
  */
  return 0;
}

