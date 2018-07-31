#include "Common.hpp"
#include "AST.hpp"

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
  text.resize(size + 1);
  fread((void*) text.c_str(), 1, size, f);
  text[size] = '\n';
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

bool runCommand(string command, bool silenced)
{
  if(silenced)
  {
    //pipe stdout and stderr to null so it doesn't go to terminal
    command += " &> /dev/null";
  }
  return system(command.c_str()) == 0;
}

string generateChar(char ch)
{
  switch(ch)
  {
    case 0:
      return "\\0";
    case '\n':
      return "\\n";
    case '\t':
      return "\\t";
    case '\r':
      return "\\r";
    case '\"':
      return "\\\"";
    case '\'':
      return "\\'";
    default:
    {
      if(isgraph(ch) || ch == ' ')
      {
        return string(1, ch);
      }
      //fall back to 8-bit hex literal
      char buf[8];
      sprintf(buf, "\\x%02hhx", ch);
      return buf;
    }
  }
}

string generateCharDotfile(char ch)
{
  switch(ch)
  {
    case 0:
      return "\\\\0";
    case '\n':
      return "\\\\n";
    case '\t':
      return "\\\\t";
    case '\r':
      return "\\\\r";
    case '\"':
      return "\\\"";
    case '\'':
      return "\\'";
    default:
    {
      if(isgraph(ch) || ch == ' ')
      {
        return string(1, ch);
      }
      //fall back to 8-bit hex literal
      char buf[8];
      sprintf(buf, "\\\\x%02hhx", ch);
      return buf;
    }
  }
}

