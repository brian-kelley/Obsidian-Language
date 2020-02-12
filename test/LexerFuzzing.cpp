#include "Common.hpp"
#include "Testing.hpp"

char genAny()
{
  //return anything but 0, since that
  //would terminate the input prematurely
  char c;
  do
  {
    c = rand();
  }
  while(c == 0);
  return c;
}

char genStandard()
{
  return ' ' + rand() % ('~' - ' ');
}

int main(int argc, const char** argv)
{
  seedRNG();
  INTERNAL_ASSERT(argc == 2);
  size_t len = 512 * 1024;
  char* text = new char[len + 1];
  text[len] = 0;
  bool doAll = !strcmp(argv[1], "--all");
  //Iterations can't be too high because even though
  //the compiler processes aren't running concurrently,
  //every fork() counts against the OS process limit
  for(int iter = 0; iter < 8; iter++)
  {
    for(size_t i = 0; i < len; i++)
    {
      if(doAll)
        text[i] = genAny();
      else
        text[i] = genStandard();
    }
    vector<string> args(1, string("-i"));
    string output = runOnyx(args, text);
    bool success = !compilerInternalError(output);
    if(!success)
    {
      cout << "Compiler produced internal error on input:\n";
      if(doAll)
      {
        //print in hex so the terminal doesn't get
        //filled with unprintable chars
        for(size_t i = 0; i < len; i++)
        {
          printf("%02hhx ", text[i]);
          if(i % 16 == 15)
            putchar('\n');
        }
      }
      else
      {
        cout << text << '\n';
      }
      return 1;
    }
    cout << "Passed iter " << iter << '\n';
  }
  delete[] text;
  return 0;
}

