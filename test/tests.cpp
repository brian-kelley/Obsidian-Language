#include "Testing.hpp"
#include "Common.hpp"

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
  try
  {
    parseProgram(op.input);
    resolveSemantics();
    vector<Expression*> mainArgs;
    if(argc > 2)
    {
      Type* charType = getCharType();
      Type* stringType = getStringType();
      Type* stringArrType = getArrayType(stringType, 1);
      vector<Expression*> stringArgs;
      for(int i = 2; i < argc; i++)
      {
        vector<Expression*> strChars;
        string s = argv[i];
        for(size_t j = 0; j < s.length(); j++)
          strChars.push_back(new IntConstant((uint64_t) s[j], charType));
        stringArgs.push_back(new CompoundLiteral(strChars, stringType));
      }
      mainArgs.push_back(new CompoundLiteral(stringArgs, stringArrType));
    }
    Interpreter(mainSubr, mainArgs);
  }
  catch(...) {}
  string gold = loadFile(testName + ".gold");
  string actual = getInterpreterOutput();
  if(actual != gold)
  {
    cout << "<<< CORRECT OUTPUT >>>\n";
    cout << gold << '\n';
    cout << "<<< ACTUAL OUTPUT >>>\n";
    cout << actual << '\n';
    cout << "Test failed, because output wrong.\n";
    cout << "Writing actual output to " << testName << ".out for diffing.\n";
    writeFile(actual, testName + ".out");
    return 1;
  }
  return 0;
}

