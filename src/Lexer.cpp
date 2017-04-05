#include "Lexer.hpp"

#define TAB_LENGTH 4

struct CodeStream
{
  CodeStream(string& srcIn) : src(srcIn)
  {
    iter = 0;
    line = 0;
    col = 0;
  }
  char getNext()
  {
    if(iter >= src.length())
      return '\0';
    char c = src[iter];
    if(c == '\n') 
    {
      line++;
      col = 0;
    }
    else if(c == '\t')
    {
      col += TAB_LENGTH;
    }
    else
    {
      col++;
    }
  }
  char peek(int ahead)
  {
    if(iter + ahead >= src.length())
      return '\0';
    return src[iter + ahead];
  }
  operator bool()
  {
    return iter == src.length();
  }
  void err(string msg)
  {
    string fullMsg = string("Lexical error at line ") + to_string(line) + ", col " + to_string(col) + ": " + msg;
    errAndQuit(fullMsg);
  }
  string& src;
  int iter;
  int line;
  int col;
};

void lex(string& code, vector<Token*>& tokList)
{
  CodeStream cs(code);
  int commentDepth = 0;
  vector<Token*> tokens;
  //note: i is incremented various amounts depending on the tokens
  while(cs)
  {
    char c = cs.getNext();
    if(c == ' ' || c == '\t' || c == '\n')
    {
      continue;
    }
    else if(c == '"')
    {
      //string literal
      int stringStart = cs.iter + 1;
      while(true)
      {
        char c = cs.getNext();
        if(c == '\\')
        {
          //eat an additional character no matter what it is
          cs.getNext();
        }
        else if(c == '"')
        {
          //end of string
          break;
        }
      }
      //stringEnd is index of the closing quotations
      int stringEnd = cs.iter;
      //get string literal between stringStart and stringEnd
      vector<char> strLit;
      strLit.reserve(stringEnd - stringStart + 1);
      for(int i = stringStart; i < stringEnd; i++)
      {
        if(code[i] == '\\')
        {
          i++;
          strLit.push_back(getEscapedChar(code[i]));
        }
        else
        {
          strLit.push_back(code[i]);
        }
      }
      strLit.push_back('\0');
      string s(&strLit[0]);
      tokList.push_back(new StrLit(s));
    }
    else if(c == ''')
    {
      char charVal = cs.getNext();
      if(charVal == '\\')
      {
        tokList.push_back(new CharLit(getEscapedChar(cs.getNext())));
      }
      else
      {
        tokList.push_back(new CharLit(charVal));
      }
      //finally, expect closing quote
      if(cs.getNext() != '\'')
      {
        cs.err("non-terminated character literal");
      }
    }
    else if(c == '/' && cs.peek(1) == '*')
    {
      commentDepth++;
      cs.getNext();
    }
    else if(c == '*' && cs.peek(1) == '/')
    {
      commentDepth--;
      if(commentDepth < 0)
      {
        cs.err("*/ without /*");
      }
      cs.getNext();
    }
    else if(isalpha(c) || c == '_')
    {
      //keyword or identifier
      //scan all following alphanumeric/underscore chars to classify
      int identStart = cs.iter;
      while(true)
      {
        char identChar = cs.getNext();
      }
    }
    {
      //???
      cout << "Lexer iter is " << i << ", have " << code.length() << " bytes of input.\n";
      cout << "Code byte at iter = " << (int) code[i] << '\n';
      cout << "Last token was \"" << tokens.back()->getStr() << "\"\n";
      string rem = code.substr(i, min<int>(10, code.length() - i));
      errAndQuit(string("Error: lexer could not identify token at index ") +
          to_string(i) + ", code: \"" + rem + "\"");
    }
  }
  if(commentDepth != 0)
  {
    cs.err("/* without */");
  }
  return tokens;
}

void addToken(vector<Token*>& tokList, string token, int hint)
{
  if(hint == IDENTIFIER)
  {
    int kw = isKeyword(token);
    if(kw != -1)
    {
      tokList.push_back(new Keyword(kw));
    }
    else
    {
      tokList.push_back(new Ident(token));
    }
  }
  else if(hint == STRING_LITERAL)
  {
    string val = "";
    for(size_t i = 1; i < token.size() - 1; i++)
    {
      if(token[i] == '\\')
      {
        if(i == token.size() - 1)
        {
          errAndQuit("String literal ends with backslash.");
        }
        val += getEscapedChar(token[i + 1]);
        i++;
      }
      else
        val += token[i];
    }
    tokList.push_back(new StrLit(val));
  }
  else if(hint == CHAR_LITERAL)
  {
    char val = token[1];
    if(val == '\\')
      val = getEscapedChar(token[2]);
    tokList.push_back(new CharLit(val));
  }
  else if(hint == INT_LITERAL)
  {
    //token is a copy outside of code stream and is null-terminated
    int val;
    sscanf(&token[0], "%i\n", &val);
    tokList.push_back(new IntLit(val));
  }
  else if(hint == PUNCTUATION)
  {
    char tok = token[0];
    //Structure punctuation
    auto it = punctMap.find(tok);
    if(it != punctMap.end())
    {
      tokList.push_back(new Punct(it->second));
    }
    else
    {
      errAndQuit(string("Invalid or unknown punctuation token: \"") + token + "\"");
    }
  }
}

char getEscapedChar(char ident)
{
  //TODO: are there more that should be supported?
  if(ident == 'n')
    return '\n';
  if(ident == 't')
    return '\t';
  if(ident == '0')
    return 0;
  if(ident == '\\')
    return '\\';
  errAndQuit(string("Unknown escape sequence: \\") + ident);
  return ' ';
}

