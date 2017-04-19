#include "Misc.hpp"
#include "Options.hpp"
#include "Utils.hpp"
#include "CGen.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "MiddleEnd.hpp"
#include "AST_Printer.hpp"
#include "MiddleEndDebug.hpp"

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
  if(op.emitC)
  {
    cout << "Will emit C code to \"" << op.outputStem + ".c" << "\"\n";
  }
  //Lexing
  vector<Token*> toks;
  lex(code, toks);
  cout << "************************************\n";
  cout << "*            TOKENS                *\n";
  cout << "************************************\n";
  for(auto& it : toks)
  {
    cout << it->getDesc() << " : " << it->getStr() << "\n";
  }
  cout << '\n';
  //Parse the global/root module
  AP(Parser::Module) ast = Parser::parseProgram(toks);
  cout << "************************************\n";
  cout << "*             AST                  *\n";
  cout << "************************************\n";
  printAST(ast);
  MiddleEnd::load(ast);
  cout << "************************************\n";
  cout << "*          Scopes/Types            *\n";
  cout << "************************************\n";
  MiddleEndDebug::printTypeTree();
  //Code generation
  generateC(op.outputStem, op.emitC, ast);
  return 0;
}

