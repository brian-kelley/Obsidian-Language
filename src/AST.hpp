#ifndef AST_H
#define AST_H

#include "Common.hpp"
#include "Token.hpp"

//List of source file paths
extern vector<string> sourceFiles;

struct IncludedFile
{
  IncludedFile() : including(-1), line(-1)
  {}
  IncludedFile(int i, int l) : including(i), line(l)
  {}
  //sourceFiles[including] is the file which includes this one
  //-1 for the root file
  int including;
  //in sourceFiles[including], which line has the #include
  //-1 for the root file
  int line;
};

extern vector<IncludedFile> includes;

struct Node
{
  Node(Token* t)
  {
    fileID = 0;
    line = t->line;
    col = t->col;
    resolved = false;
  }
  //Do full context-sensitive semantic checking
  //Some nodes don't need to be checked, so provide empty default definition
  virtual void resolve(bool) {};
  void finalResolve()
  {
    resolve(true);
  }
  bool tryResolve()
  {
    resolve(false);
    return resolved;
  }
  void setLocation(Node* other)
  {
    fileID = other->fileID;
    line = other->line;
    col = other->col;
  }
  //Convert Node back into tokens (used by emit statements)
  virtual vector<Token*> unparse() = 0;
  //All nodes know their position in code (for error messages)
  int fileID; //index in sourceFiles
  int line;
  int col;
  bool resolved;
};

#endif

