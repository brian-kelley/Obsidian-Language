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
  void accept();                //accept (and discard) any token
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
  string expectIdent();
  Token* lookAhead(int n = 0);  //get the next token without advancing pos
  void err(string msg = "");

  struct Member
  {
    vector<string> names;
  };

  void parseScopedDecl(Scope* s, bool semicolon);
  //parse a statement, but don't add it to block
  Statement* parseStatement(Block* b, bool semicolon);
  //parse a statement or declaration
  //if statement, return it but don't add to block
  //if decl, add it to block's scope
  Statement* parseStatementOrDecl(Block* b, bool semicolon);
  If* parseIf(Block* b);
  While* parseWhile(Block* b);
  //parse a variable declaration, and add the variable to scope
  //if s belongs to a block and the variable is initialized, return the assignment
  Assign* parseVarDecl(Scope* s, bool semicolon);
  Subroutine* parseSubroutine(Scope* s);
  ExternalSubroutine* parseExternalSubroutine(Scope* s);
  Expression* parseExpression(Scope* s);
  void parseModule(Scope* s);
  void parseAlias(Scope* s);
  ForC* parseForC(Block* b);
  ForArray* parseForArray(Block* b);
  ForRange* parseForRange(Block* b);
  Member* parseMember();
  //Parse a block (which has already been constructed)
  void parseBlock(Block* b);
  void parseTest(Scope* s);
  Type* parseType(Scope* s);
}

//Utils
ostream& operator<<(ostream& os, const Parser::Member& mem);
ostream& operator<<(ostream& os, const Parser::ParseNode& pn);

#endif

