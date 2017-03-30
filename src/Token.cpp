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
  SET_KEY("uchar", UCHAR)
  SET_KEY("short", SHORT)
  SET_KEY("ushort", USHORT)
  SET_KEY("int", INT)
  SET_KEY("uint" UINT)
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
  SET_KEY("metaif", METAIF)
  SET_KEY("metaelse", METAELSE)
  SET_KEY("metafor", METAFOR)
  SET_KEY("metafunc", METAFUNC)
  SET_KEY("variant", VARIANT)
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
  punctTable.resize(punctMap.size());
  for(auto& it : punctMap)
  {
    punctTable[it.second] = it.first;
  }
}

int isKeyword(string str)
{
  auto it = keywordTable.find(str);
  if(it == keywordTable.end())
    return -1;
  else
    return (it - keywordTabler.begin())->second;
}

/* Identifier */
Ident::Ident(string name)
{
  this->name = name;
}

bool Ident::operator==(const Ident& rhs)
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
Oper::Oper(int op)
{
  this->op = op;
}

bool Oper::operator==(const Oper& rhs)
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
StrLit::StrLit(string val)
{
  this->val = val;
}

bool StrLit::operator==(const StrLit& rhs)
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
CharLit::CharLit(char val)
{
  this->val = val;
}

bool CharLit::operator==(const CharLit& rhs)
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
IntLit::IntLit(int val)
{
  this->val = val;
}

bool IntLit::operator==(const IntLit& rhs)
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
FloatLit::FloatLit(double val)
{
  this->val = val;
}

bool FloatLit::operator==(const FloatLit& rhs)
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
Punct::Punct(PUNC val)
{
  this->val = val;
}

bool Punct::operator==(const Punct& rhs)
{
  return val == rhs.val;
}

int Punct::getType()
{
  return PUNCTUATION;
}

string Punct::getStr()
{
  return punctTable[val];
}

string Punct::getDesc()
{
  return tokTypetable[PUNCTUATION];
}

/* Keyword */
Keyword::Keyword(string text)
{
  int val = isKeyword(text);
  if(val == -1)
  {
    errAndQuit("Expected a keyword.");
  }
  this->kw = val;
}

Keyword::Keyword(int val)
{
  this->kw = val;
}

bool Keyword::operator==(const Keyword& rhs)
{
  return kw == rhs.kw;
}

int Keyword::getType()
{
  return KEYWORD;
}

string Keyword::getStr()
{
  return keywordTable[val];
}

string Keyword::getDesc()
{
  return tokTypeTable[KEYWORD];
}

bool PastEOF::operator==(const PastEOF& rhs)
{
  return true;
}

int PastEOF::getType()
{
  return PAST_EOF;
}

string PastEOF::getStr()
{
  return "";
}

string PastEOF::getDesc()
{
  return tokTypeTable[PAST_EOF];
}

