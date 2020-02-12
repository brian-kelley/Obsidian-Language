#include "Common.hpp"
#include "AST.hpp"
#include "SourceFile.hpp"

bool runCommand(string command, bool silenced)
{
  if(silenced)
  {
    //pipe stdout and stderr to null so it doesn't go to terminal
    command += " &> /dev/null";
  }
  return system(command.c_str()) == 0;
}

static bool verbose_enabled = false;

void enableVerboseMode()
{
  verbose_enabled = true;
}

bool verboseEnabled()
{
  return verbose_enabled;
}

string getSourceName(int id)
{
  return sourceFileFromID(id)->path;
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

