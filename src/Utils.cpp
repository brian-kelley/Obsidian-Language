#include "Utils.hpp"

string loadFile(string filename)
{
  FILE* f = fopen(filename.c_str(), "rb");
  if(!f)
  {
    errAndQuit(string("Could not open file \"") + filename + "\" for reading.");
  }
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  rewind(f);
  string text;
  text.resize(size + 2);
  fread((void*) text.c_str(), 1, size, f);
  text[size + 1] = '\n';
  fclose(f);
  return text;
}

void writeFile(string& text, string filename)
{
  FILE* f = fopen(filename.c_str(), "wb");
  if(!f)
  {
    errAndQuit(string("Could not open file \"") + filename + "\" for writing.");
  }
  fwrite(text.c_str(), 1, text.size(), f);
  fclose(f);
}

void errAndQuit(string message)
{
  cout << message << '\n';
  exit(EXIT_FAILURE);
}

