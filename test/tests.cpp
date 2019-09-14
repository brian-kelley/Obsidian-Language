#include "Common.hpp"
#include "Options.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "AST.hpp"
#include "AstInterpreter.hpp"
#include "BuiltIn.hpp"

Module* global = nullptr;

int main(int argc, const char** argv)
{
  cout << "Hello from test main.\n";
  if(argc != 2)
    return 1;
  string testName(argv[1]);
  string srcFile = testName + ".os";
  const char* compilerArgv[2] = {"onyx", srcFile.c_str()};
  Options op = parseOptions(2, compilerArgv);
  initTokens();
  global = new Module("", nullptr);
  createBuiltinTypes();
  //Parse the global/root module
  parseProgram(op.input);
  global->resolve();
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
  Interpreter(mainSubr, mainArgs);
  string gold = loadFile(testName + ".gold");
  string actual = getInterpreterOutput();
  if(actual != gold)
  {
    cout << "<<< CORRECT OUTPUT >>>\n";
    cout << gold << '\n';
    cout << "<<< ACTUAL OUTPUT >>>\n";
    cout << actual << '\n';
    cout << "Test failed, because output wrong.\n";
    return 1;
  }
  return 0;
}
