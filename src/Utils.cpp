#include "Utils.hpp"

Text::Text(size_t n)
{
  buf = new char[n];
  size = n;
}

Text::Text(const Text& t)
{
  buf = new char[t.size];
  size = t.size;
}

Text::~Text()
{
  if(buf)
    delete buf;
}

char* Utils::loadFile(const char* filename)
{
  FILE* f = fopen(filename, "r");
  if(!f)
  {
    printf("Could not open source file: \"%s\"\n", filename);
    exit(EXIT_FAILURE);
  }
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  rewind(f);
  Text t(size + 2);
  fread(t.buf, 1, size, f);
  t.buf[size] = '\n';
  t.buf[size + 1] = 0;
  fclose(f);
  return t;
}

