#include "AST.hpp"

vector<string> sourceFiles;
vector<IncludedFile> includes;

string Node::printLocation()
{
  Oss oss;
  oss << sourceFiles[fileID] << ", " << line << ':' << col;
  return oss.str();
}

