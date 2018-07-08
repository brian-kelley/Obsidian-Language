#ifndef AST_H
#define AST_H

#include "Common.hpp"

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
  Node()
  {
    fileID = 0;
    line = 0;
    col = 0;
    resolved = false;
  }
  //Do full context-sensitive semantic checking
  //Some nodes don't need to be checked, so provide empty default definition
  //resolveImpl only does the type-specific logic to resolve a node, and set
  //resolved = true on success.
  //
  //Externally, finalResolve or tryResolve should be used instead.
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
  //All nodes know their position in code (for error messages)
  int fileID; //index in sourceFiles
  int line;
  int col;
  string printLocation();
  bool resolved;
  bool resolving;

  void resolve(bool final)
  {
    if(resolved)
      return;
    if(resolving)
    {
      errMsgLoc(this, "Circular definition in program");
    }
    resolving = true;
    resolveImpl(final);
    resolving = false;
  }

  virtual void resolveImpl(bool) {};
};

#endif

