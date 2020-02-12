#include "Testing.hpp"
#include "Utils.hpp"

int main(int argc, const char** argv)
{
  INTERNAL_ASSERT(argc == 2);
  string fileStem = argv[1];
  string srcFile = fileStem + ".os";
  string goldOut = loadFile(fileStem + ".gold");
  vector<string> args(1, srcFile);
  string actualOut = runOnyx(args, "");
  bool success = actualOut == goldOut;
  if(success)
    cout << "TEST PASSED\n";
  else
  {
    cout << "TEST FAILED\n";
    cout << "Correct output:\n";
    cout << goldOut << "<<<\n";
    cout << "Produced output:\n";
    cout << actualOut << "<<<\n";
    cout << '\n';
  }
  return success ? 0 : 1;
}

