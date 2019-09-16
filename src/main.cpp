#include "Common.hpp"
#include "Options.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "AST.hpp"
#include "AST_Output.hpp"
#include "AstInterpreter.hpp"
#include "BuiltIn.hpp"

//#include "C_Backend.hpp"
//#include "IR.hpp"
//#include "Dataflow.hpp"
//#include "IRDebug.hpp"

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
  if(!mainSubr)
  {
    errMsg("Program requires main procedure to be defined");
  }
}

int main(int argc, const char** argv)
{
  auto startTime = clock();
  Options op = parseOptions(argc, argv);
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
  outputAST(global, "AST.dot");
  vector<Expression*> mainArgs;
  if(argc > 2)
  {
    Type* stringType = getArrayType(primitives[Prim::CHAR], 1);
    Type* stringArrType = getArrayType(primitives[Prim::CHAR], 2);
    vector<Expression*> stringArgs;
    for(int i = 2; i < argc; i++)
    {
      vector<Expression*> strChars;
      string s = argv[i];
      for(size_t j = 0; j < s.length(); j++)
        strChars.push_back(new CharConstant(s[j]));
      stringArgs.push_back(new CompoundLiteral(strChars, stringType));
    }
    mainArgs.push_back(new CompoundLiteral(stringArgs, stringArrType));
  }
  TIMEIT("Interpreting AST", Interpreter(mainSubr, mainArgs));
    /*
  TIMEIT("IR/CFG construction", IR::buildIR();)
  //DEBUG_DO(IRDebug::dumpIR("ir.dot");)
  TIMEIT("Optimizing IR", IR::optimizeIR();)
  //TIMEIT("C generate & compile", C::generate(op.outputStem, true););
  //Code generation
  auto elapsed = (double) (clock() - startTime) / CLOCKS_PER_SEC;
  cout << "Compilation completed in " << elapsed << " seconds.\n";
  //int lines = PastEOF::inst.line;
  cout << "Processed " << lines << " lines (" << lines / elapsed;
  cout << " lines/s, or " << code.length() / (elapsed * 1000000) << " MB/s)\n";
  */
  return 0;
}

