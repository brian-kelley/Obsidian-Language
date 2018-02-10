#ifndef AST_H
#define AST_H

#include "Common.hpp"
#include "Token.hpp"

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
  //Convert Node back into tokens (used by emit statements)
  virtual vector<Token*> unparse() = 0;
  //Nodes know their own position in code
  //TODO: use fileID in future, when multiple files are allowed
  int fileID;
  int line;
  int col;
  bool resolved;
};

#endif

