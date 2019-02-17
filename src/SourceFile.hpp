#ifndef SOURCE_FILE_H
#define SOURCE_FILE_H

#include "Common.hpp"

struct Module;
struct Token;
struct Node;

struct SourceFile
{
  SourceFile(Node* includeLoc, string path);
  vector<Token*> tokens;
  string path;
  int id;
};

//Look up the loaded source file with given path
//If it has already been loaded, no I/O is done
SourceFile* getSourceFile(Node* includeLoc, string path);
SourceFile* sourceFileFromID(int id);

#endif
