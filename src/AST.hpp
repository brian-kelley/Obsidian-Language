#ifndef AST_H
#define AST_H

#include "Common.hpp"

struct Node
{
  Node()
  {
    fileID = 0;
    line = 0;
    col = 0;
    resolved = false;
    resolving = false;
  }
  //Do full context-sensitive semantic checking
  //Some nodes don't need to be checked, so provide empty default definition
  //resolveImpl only does the type-specific logic to resolve a node, and set
  //resolved = true on success.
  //
  //Externally, finalResolve or tryResolve should be used instead.
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

  void resolve()
  {
    if(resolved)
      return;
    if(resolving)
    {
      cout << "Circular definition at " << printLocation() << "!\n";
      INTERNAL_ERROR;
      //errMsgLoc(this, "Circular definition in program");
    }
    resolving = true;
    resolveImpl();
    resolving = false;
  }

  virtual void resolveImpl() {};
};

struct Member : public Node
{
  Member() {}
  Member(string s) : names(1, s) {}
  Member(const vector<string>& n) : names(n) {}
  vector<string> names;
};

#endif

