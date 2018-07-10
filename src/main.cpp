#include "Common.hpp"
#include "Options.hpp"
#include "C_Backend.hpp"
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
}

void semanticCheck(Module* program)
{
  program->finalResolve();
  if(!programHasMain)
  {
    errMsg("Program requires main procedure to be defined");
  }
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
  //string code = getBuiltins() + loadFile(op.input);
  string code = loadFile(op.input);
  DEBUG_DO(cout << "Compiling " << code.size() << " bytes of source code, including builtins\n";);
  //Lexing
  sourceFiles.push_back(op.input);
  //empty "includes" is for root file (no other file is including it)
  includes.emplace_back();
  TIMEIT("Lexing", Parser::tokens = lex(code, 0););
  /*
  //print tokens
  cout << "Tokens:\n";
  for(auto& tok : Parser::tokens)
  {
    cout << tok->getStr() << '\n';
  }
  */
  //Parse the global/root module
  TIMEIT("Parsing", Parser::parseProgram(););
  //DEBUG_DO(outputAST(global, "parse.dot"););
  TIMEIT("Semantic analysis", global->finalResolve(););
  DEBUG_DO(outputAST(global, "AST.dot");)
  TIMEIT("IR/CFG construction", IR::buildIR();)
  DEBUG_DO(IRDebug::dumpIR("ir.dot");)
  Liveness::buildAllLivesets();
  //TIMEIT("C generate & compile", C::generate(op.outputStem, true););
  //Code generation
  auto elapsed = (double) (clock() - startTime) / CLOCKS_PER_SEC;
  cout << "Compilation completed in " << elapsed << " seconds.\n";
  int lines = PastEOF::inst.line;
  cout << "Processed " << lines << " lines (" << lines / elapsed;
  cout << " lines/s, or " << code.length() / (elapsed * 1000000) << " MB/s)\n";
  return 0;
}

