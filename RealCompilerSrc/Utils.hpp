#include <cstdlib>
#include <cstdio>

struct Text
{
  Text(size_t n);
  Text(const Text& t);
  ~Text();
  char* buf;
  size_t size;
}

//Allocate a buffer and load a file into it
//Always append one newline and a \0 at the end
Text loadFile(const char* filename);

