#include "Misc.hpp"
#include "Token.hpp"

//note: code must already be preprocessed
vector<Token*> lex(string& code);
void addToken(vector<Token*>& tokList, string token, int hint);
char getEscapedChar(char ident);

