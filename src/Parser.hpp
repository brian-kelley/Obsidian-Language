#ifndef PARSER_H
#define PARSER_H

#include "Common.hpp"
#include "Token.hpp"
#include "Scope.hpp"

namespace Parser
{
  //Token stream management
  extern vector<Token*> tokens;

  /*
  string emitBuffer;
  //execute the meta-statement starting at offset start in code stream
  void metaStatement(size_t start);
  void emit(string source);
  void emit(ParseNode* nonterm);
  void emit(Token* tok);
  */

  //Parse a program from token string (only parsing function called from main)
  Module* parseProgram();

  //Token stream & utilities
  extern size_t pos;                //token iterator
  extern vector<Token*> tokens;    //all tokens from lexer

  void unget();                 //back up one token (no-op if at start of token string)
  void accept();                //accept any token
  bool accept(Token& t);
  Token* accept(int tokType);   //return NULL if tokType doesn't match next
  bool acceptKeyword(int type);
  bool acceptOper(int type);
  bool acceptPunct(int type);
  void expect(Token& t);
  Token* expect(int tokType);
  void expectKeyword(int type);
  void expectOper(int type);
  void expectPunct(int type);
  Token* lookAhead(int n = 0);  //get the next token without advancing pos
  void err(string msg = "");

  struct Member
  {
    vector<string> names;
  };

  void parseScopedDecl(Scope* s, bool semicolon);
  void parseStatement(Block* b);
  void parseStatementOrDecl(Block* b);
  void parseVarDecl(Scope* s);

  void parseBlock(Scope* s);
  void parseBlock(Subroutine* s);
  void parseBlock(For* f);
  void parseBlock(While* w);
}

//Utils
ostream& operator<<(ostream& os, const Parser::Member& mem);
ostream& operator<<(ostream& os, const Parser::ParseNode& pn);

#endif

