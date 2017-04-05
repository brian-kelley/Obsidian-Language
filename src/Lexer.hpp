#include "Misc.hpp"
#include "Token.hpp"

//Lex source file contents
void lex(string& code, vector<Token*>& tokList);
void addToken(vector<Token*>& tokList, string token, int hint);
char getEscapedChar(char ident);

