#include "AST.hpp"

vector<string> sourceFiles;
vector<IncludedFile> includes;

string Node::printLocation()
{
  return sourceFiles[fileID] + ", " + to_string(line) + ":" + to_string(col);
}

