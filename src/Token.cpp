#include "Token.hpp"

PastEOF PastEOF::inst;
map<int, string> keywordTable;
map<int, string> tokTypeTable;

void initTokens()
{
#define SET_KEY(str, val) keywordTable[val] = str;
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
#undef SET_KEY
  tokTypeTable[IDENTIFIER] = "identifier";
  tokTypeTable[STRING_LITERAL] = "string-literal";
  tokTypeTable[CHAR_LITERAL] = "char-literal";
  tokTypeTable[INT_LITERAL] = "int-literal";
  tokTypeTable[FLOAT_LITERAL] = "float-literal";
  tokTypeTable[PUNCTUATION] = "punctuation";
  tokTypeTable[OPERATOR] = "operator";
  tokTypeTable[KEYWORD] = "keyword";
  tokTypeTable[PAST_EOF] = "null-token";
}

int isKeyword(string str)
{
  for(auto& it : keywordMap)
  {
    if(it.second == str)
      return it.first;
  }
  return -1;
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
  switch(op)
  {
    case PLUS:
      return "+";
    case PLUSEQ:
      return "+=";
    case SUB:
      return "-";
    case SUBEQ:
      return "-=";
    case MUL:
      return "*";
    case MULEQ:
      return "*=";
    case DIV:
      return "/";
    case DIVEQ:
      return "/=";
    case LOR:
      return "||";
    case BOR:
      return "|";
    case BXOR:
      return "^";
    case LNOT:
      return "!";
    case BNOT:
      return "~";
    case LAND:
      return "&&";
    case BAND:
      return "&";
    case SHL:
      return "<<";
    case SHR:
      return ">>";
    case CMPEQ:
      return "==";
    case CMPNEQ:
      return "!=";
    case CMPL:
      return "<";
    case CMPLE:
      return "<=";
    case CMPG:
      return ">";
    case CMPGE:
      return ">=";
    case LBRACK:
      return "[";
    case RBRACK:
      return "]";
    case ASSIGN:
      return "=";
    default:
      return "";
  }
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
  switch(val)
  {
    case SEMICOLON:
      return ";";
    case COLON:
      return ":";
    case LPAREN:
      return "(";
    case RPAREN:
      return ")";
    case LBRACE:
      return "{";
    case RBRACE:
      return "}";
    case DOT:
      return ".";
    case COMMA:
      return ",";
    case DOLLAR:
      return "$";
    default:
      return "";
  };
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

