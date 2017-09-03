#include "Token.hpp"

PastEOF PastEOF::inst;

map<string, int> keywordMap;
vector<string> keywordTable;
map<string, int> operatorMap;
vector<string> operatorTable;
map<char, int> punctMap;
vector<char> punctTable;
//enum values => string
vector<string> tokTypeTable;

void initTokens()
{
#define SET_KEY(str, val) keywordMap[str] = val;
  SET_KEY("void", VOID)
  SET_KEY("bool", BOOL)
  SET_KEY("char", CHAR)
  SET_KEY("byte", BYTE)
  SET_KEY("ubyte", UBYTE)
  SET_KEY("short", SHORT)
  SET_KEY("ushort", USHORT)
  SET_KEY("int", INT)
  SET_KEY("uint", UINT)
  SET_KEY("long", LONG)
  SET_KEY("ulong", ULONG)
  SET_KEY("string", STRING)
  SET_KEY("float", FLOAT)
  SET_KEY("double", DOUBLE)
  SET_KEY("print", PRINT)
  SET_KEY("return", RETURN)
  SET_KEY("typedef", TYPEDEF)
  SET_KEY("struct", STRUCT)
  SET_KEY("this", THIS)
  SET_KEY("func", FUNC)
  SET_KEY("proc", PROC)
  SET_KEY("error", ERROR)
  SET_KEY("trait", TRAIT)
  SET_KEY("if", IF)
  SET_KEY("else", ELSE)
  SET_KEY("for", FOR)
  SET_KEY("while", WHILE)
  SET_KEY("switch", SWITCH)
  SET_KEY("case", CASE)
  SET_KEY("default", DEFAULT)
  SET_KEY("break", BREAK)
  SET_KEY("continue", CONTINUE)
  SET_KEY("union", UNION)
  SET_KEY("auto", AUTO)
  SET_KEY("module", MODULE)
  SET_KEY("enum", ENUM)
  SET_KEY("using", USING)
  SET_KEY("true", TRUE)
  SET_KEY("false", FALSE)
  SET_KEY("assert", ASSERT)
  SET_KEY("test", TEST)
  SET_KEY("functype", FUNCTYPE)
  SET_KEY("proctype", PROCTYPE)
  SET_KEY("nonterm", NONTERM)
  SET_KEY("T", T_TYPE);
  SET_KEY("static", STATIC);
  SET_KEY("array", ARRAY);
  keywordTable.resize(keywordMap.size());
  for(auto& it : keywordMap)
  {
    keywordTable[it.second] = it.first;
  }
#undef SET_KEY
  tokTypeTable.resize(NUM_TOKEN_TYPES);
  tokTypeTable[IDENTIFIER] = "identifier";
  tokTypeTable[STRING_LITERAL] = "string-literal";
  tokTypeTable[CHAR_LITERAL] = "char-literal";
  tokTypeTable[INT_LITERAL] = "int-literal";
  tokTypeTable[FLOAT_LITERAL] = "float-literal";
  tokTypeTable[PUNCTUATION] = "punctuation";
  tokTypeTable[OPERATOR] = "operator";
  tokTypeTable[KEYWORD] = "keyword";
  tokTypeTable[PAST_EOF] = "null-token";
  operatorMap["+"] = PLUS;
  operatorMap["+="] = PLUSEQ;
  operatorMap["-"] = SUB;
  operatorMap["-="] = SUBEQ;
  operatorMap["*"] = MUL;
  operatorMap["*="] = MULEQ;
  operatorMap["/"] = DIV;
  operatorMap["/="] = DIVEQ;
  operatorMap["%"] = MOD;
  operatorMap["%="] = MODEQ;
  operatorMap["||"] = LOR;
  operatorMap["|"] = BOR;
  operatorMap["|="] = BOREQ;
  operatorMap["^"] = BXOR;
  operatorMap["^="] = BXOREQ;
  operatorMap["!"] = LNOT;
  operatorMap["~"] = BNOT;
  operatorMap["&&"] = LAND;
  operatorMap["&"] = BAND;
  operatorMap["&="] = BANDEQ;
  operatorMap["<<"] = SHL;
  operatorMap["<<="] = SHLEQ;
  operatorMap[">>"] = SHR;
  operatorMap[">>="] = SHREQ;
  operatorMap["=="] = CMPEQ;
  operatorMap["!="] = CMPNEQ;
  operatorMap["<"] = CMPL;
  operatorMap["<="] = CMPLE;
  operatorMap[">"] = CMPG;
  operatorMap[">="] = CMPGE;
  operatorMap["="] = ASSIGN;
  operatorMap["++"] = INC;
  operatorMap["--"] = DEC;
  operatorTable.resize(operatorMap.size());
  for(auto& it : operatorMap)
  {
    operatorTable[it.second] = it.first;
  }
  punctMap[';'] = SEMICOLON;
  punctMap[':'] = COLON;
  punctMap['('] = LPAREN;
  punctMap[')'] = RPAREN;
  punctMap['{'] = LBRACE;
  punctMap['}'] = RBRACE;
  punctMap['['] = LBRACKET;
  punctMap[']'] = RBRACKET;
  punctMap['.'] = DOT;
  punctMap[','] = COMMA;
  punctMap['$'] = DOLLAR;
  punctMap['?'] = QUESTION;
  punctTable.resize(punctMap.size());
  for(auto& it : punctMap)
  {
    punctTable[it.second] = it.first;
  }
}

int isKeyword(string str)
{
  auto it = keywordMap.find(str);
  if(it == keywordMap.end())
    return -1;
  else
    return it->second;
}

Token::Token()
{
  type = -1;
}

/* Identifier */
Ident::Ident()
{
  type = IDENTIFIER;
}

Ident::Ident(string n)
{
  type = IDENTIFIER;
  this->name = n;
}

bool Ident::compareTo(Token* rhs)
{
  if(rhs->getType() == IDENTIFIER && ((Ident*) rhs)->name == name)
    return true;
  return false;
}

bool Ident::operator==(Ident& rhs)
{
  return name == rhs.name;
}

int Ident::getType()
{
  return IDENTIFIER;
}

string Ident::getStr()
{
  if(name.length())
  {
    return name;
  }
  else
  {
    return "identifier";
  }
}

string Ident::getDesc()
{
  return tokTypeTable[IDENTIFIER];
}

/* Operator */
Oper::Oper()
{
  type = OPERATOR;
}

Oper::Oper(int o)
{
  type = OPERATOR;
  this->op = o;
}

bool Oper::compareTo(Token* rhs)
{
  return rhs->getType() == OPERATOR && ((Oper*) rhs)->op == op;
}

bool Oper::operator==(Oper& rhs)
{
  return op == rhs.op;
}

int Oper::getType()
{
  return OPERATOR;
}

string Oper::getStr()
{
  return operatorTable[op];
}

string Oper::getDesc()
{
  return tokTypeTable[OPERATOR];
}

/* String Literal */
StrLit::StrLit()
{
  type = STRING_LITERAL;
}

StrLit::StrLit(string v)
{
  type = STRING_LITERAL;
  this->val = v;
}

bool StrLit::compareTo(Token* rhs)
{
  return rhs->getType() == STRING_LITERAL && ((StrLit*) rhs)->val == val;
}

bool StrLit::operator==(StrLit& rhs)
{
  return val == rhs.val;
}

int StrLit::getType()
{
  return STRING_LITERAL;
}

string StrLit::getStr()
{
  return string("\"") + val + "\"";
}

string StrLit::getDesc()
{
  return tokTypeTable[STRING_LITERAL];
}

/* Character Literal */
CharLit::CharLit()
{
  type = CHAR_LITERAL;
}

CharLit::CharLit(char v)
{
  type = CHAR_LITERAL;
  this->val = v;
}

bool CharLit::compareTo(Token* rhs)
{
  return rhs->getType() == CHAR_LITERAL && ((CharLit*) rhs)->val == val;
}

bool CharLit::operator==(CharLit& rhs)
{
  return val == rhs.val;
}

int CharLit::getType()
{
  return CHAR_LITERAL;
}

string CharLit::getStr()
{
  return string("'") + val + "'";
}

string CharLit::getDesc()
{
  return tokTypeTable[CHAR_LITERAL];
}

/* Integer Literal */
IntLit::IntLit()
{
  type = INT_LITERAL;
}

IntLit::IntLit(uint64_t v)
{
  type = INT_LITERAL;
  this->val = v;
}

bool IntLit::compareTo(Token* rhs)
{
  return rhs->getType() == INT_LITERAL && ((IntLit*) rhs)->val == val;
}

bool IntLit::operator==(IntLit& rhs)
{
  return val == rhs.val;
}

int IntLit::getType()
{
  return INT_LITERAL;
}

string IntLit::getStr()
{
  return to_string(val);
}

string IntLit::getDesc()
{
  return tokTypeTable[INT_LITERAL];
}

/* float/double literal */
FloatLit::FloatLit()
{
  type = FLOAT_LITERAL;
}

FloatLit::FloatLit(double v)
{
  type = FLOAT_LITERAL;
  this->val = v;
}

bool FloatLit::compareTo(Token* rhs)
{
  return rhs->getType() == FLOAT_LITERAL && ((FloatLit*) rhs)->val == val;
}

bool FloatLit::operator==(FloatLit& rhs)
{
  return val == rhs.val;
}

int FloatLit::getType()
{
  return FLOAT_LITERAL;
}

string FloatLit::getStr()
{
  return to_string(val);
}

string FloatLit::getDesc()
{
  return tokTypeTable[FLOAT_LITERAL];
}

/* Punctuation */
Punct::Punct()
{
  type = PUNCTUATION;
}

Punct::Punct(int v)
{
  type = PUNCTUATION;
  this->val = v;
}

bool Punct::compareTo(Token* rhs)
{
  return rhs->getType() == PUNCTUATION && ((Punct*) rhs)->val == val;
}

bool Punct::operator==(Punct& rhs)
{
  return val == rhs.val;
}

int Punct::getType()
{
  return PUNCTUATION;
}

string Punct::getStr()
{
  return string("") + punctTable[val];
}

string Punct::getDesc()
{
  return tokTypeTable[PUNCTUATION];
}

/* Keyword */
Keyword::Keyword()
{
  type = KEYWORD;
}

Keyword::Keyword(string text)
{
  type = KEYWORD;
  int val = isKeyword(text);
  if(val == -1)
  {
    ERR_MSG("Expected a keyword.");
  }
  this->kw = val;
}

Keyword::Keyword(int val)
{
  this->kw = val;
}

bool Keyword::compareTo(Token* rhs)
{
  return rhs->getType() == KEYWORD && ((Keyword*) rhs)->kw == kw;
}

bool Keyword::operator==(Keyword& rhs)
{
  return kw == rhs.kw;
}

int Keyword::getType()
{
  return KEYWORD;
}

string Keyword::getStr()
{
  return keywordTable[kw];
}

string Keyword::getDesc()
{
  return tokTypeTable[KEYWORD];
}

PastEOF::PastEOF()
{
  type = PAST_EOF;
}

bool PastEOF::compareTo(Token* t)
{
  return t->getType() == PAST_EOF;
}

bool PastEOF::operator==(PastEOF& rhs)
{
  return true;
}

int PastEOF::getType()
{
  return PAST_EOF;
}

string PastEOF::getStr()
{
  return "<INVALID TOKEN>";
}

string PastEOF::getDesc()
{
  return tokTypeTable[PAST_EOF];
}

