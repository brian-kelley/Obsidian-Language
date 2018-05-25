#include "Common.hpp"
#include "Options.hpp"
#include "C_Backend.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "AST.hpp"
//#include "ParseTreeOutput.hpp"
#include "BuiltIn.hpp"

void init()
{
  //all namespace initialization
  initTokens();
}

int main(int argc, const char** argv)
{
  auto startTime = clock();
  init();
  Options op = parseOptions(argc, argv);
  if(argc == 1)
  {
    puts("Error: no input files.");
    return EXIT_FAILURE;
  }
  //all program code: prepend builtin code to source file
  string code = getBuiltins() + loadFile(op.input);
  DEBUG_DO(cout << "Compiling " << code.size() << " bytes of source code, including builtins\n";);
  //Lexing
  sourceFiles.push_back(op.input);
  //empty "includes" is for root file (no other file is including it)
  includes.emplace_back();
  TIMEIT("Lexing", Parser::tokens = lex(code, 0););
  //Parse the global/root module
  Module* program;
  TIMEIT("Parsing", program = Parser::parseProgram(););
  //DEBUG_DO(outputParseTree(parseTree, "parse.dot"););
  TIMEIT("Semantic analysis", program->finalResolve(););
  TIMEIT("C generate & compile", C::generate(op.outputStem, true););
  //Code generation
  auto elapsed = (double) (clock() - startTime) / CLOCKS_PER_SEC;
  cout << "Compilation completed in " << elapsed << " seconds.\n";
  int lines = PastEOF::inst.line;
  cout << "Processed " << lines << " lines (" << lines / elapsed;
  cout << " lines/s, or " << code.length() / (elapsed * 1000000) << " MB/s)\n";
  return 0;
}

