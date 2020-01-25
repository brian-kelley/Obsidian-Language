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

void resolveSemantics()
{
  global->resolve();
  if(!mainSubr)
  {
    errMsg("Program requires proc main to be defined");
  }
}

int main(int argc, const char** argv)
{
  //auto startTime = clock();
  Options op = parseOptions(argc, argv);
  //init creates the builtin declarations
  init();
  //all program code: prepend builtin code to source file
  //string code = getBuiltins() + loadFile(op.input);
  //DEBUG_DO(cout << "Compiling " << code.size() << " bytes of source code, including builtins\n";);
  //Lexing
  //Parse the global/root module
  if(op.verbose)
    enableVerboseMode();
  if(op.interactive)
    TIMEIT("Parsing", parseProgram();)
  else
    TIMEIT("Parsing", parseProgram(op.input);)
  //DEBUG_DO(outputAST(global, "parse.dot"););
  TIMEIT("Semantic analysis", resolveSemantics(););
  outputAST(global, "AST.dot");
  vector<Expression*> mainArgs;
  Type* stringType = getStringType();
  Type* stringArrType = getArrayType(stringType, 1);
  vector<Expression*> stringArgs;
  for(auto& s : op.interpArgs)
  {
    vector<Expression*> strChars;
    for(size_t j = 0; j < s.length(); j++)
      strChars.push_back(new IntConstant((uint64_t) s[j], getCharType()));
    stringArgs.push_back(new CompoundLiteral(strChars, stringType));
  }
  if(stringArgs.size())
  {
    mainArgs.push_back(new CompoundLiteral(stringArgs, stringArrType));
  }
  TIMEIT("Interpreting AST", Interpreter(mainSubr, mainArgs));
  return 0;
}

