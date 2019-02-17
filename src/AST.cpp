#include "AST.hpp"
#include "SourceFile.hpp"

string Node::printLocation()
{
  return sourceFileFromID(fileID)->path + ", " + to_string(line) + ":" + to_string(col);
}

