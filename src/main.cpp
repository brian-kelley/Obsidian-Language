#include "Common.hpp"
#include "Options.hpp"
#include "C_Backend.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "ParseTreeOutput.hpp"
#include "MiddleEnd.hpp"
//#include "MiddleEndDebug.hpp"

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
  string code = loadFile(op.input.c_str());
  DEBUG_DO(cout << "Loaded " << code.size() << " bytes of source code.\n";);
  //Lexing
  vector<Token*> toks;
  TIMEIT("Lexing", lex(code, toks););
  /*
  DEBUG_DO({
    cout << "************************************\n";
    cout << "*            TOKENS                *\n";
    cout << "************************************\n";
    for(auto& it : toks)
    {
      cout << it->getDesc() << " : " << it->getStr() << "\n";
    }
    cout << '\n';
  });
  */
  //Parse the global/root module
  Parser::Module* parseTree;
  TIMEIT("Parsing", parseTree = Parser::parseProgram(toks););
  DEBUG_DO(outputParseTree(parseTree, "parse.dot"););
  TIMEIT("Middle end", MiddleEnd::load(parseTree););
  /*
  DEBUG_DO({
    cout << "************************************\n";
    cout << "*          Scopes/Types            *\n";
    cout << "************************************\n";
    MiddleEndDebug::printTypeTree();
    cout << "************************************\n";
    cout << "*          Subroutines             *\n";
    cout << "************************************\n";
    MiddleEndDebug::printSubroutines();
  });
  */
  DEBUG_DO(cout << "Running C backend.\n";)
  TIMEIT("C generate & compile", C::generate(op.outputStem, true););
  //Code generation
  auto elapsed = (double) (clock() - startTime) / CLOCKS_PER_SEC;
  cout << "Compilation completed in " << elapsed << " seconds.\n";
  int lines = PastEOF::inst.line;
  cout << "Processed " << lines << " lines (" << lines / elapsed << " lines/s, or " << code.length() / (elapsed * 1000000) << " MB/s)\n";
  return 0;
}

