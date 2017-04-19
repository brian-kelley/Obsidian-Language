#include "Lexer.hpp"

#define TAB_LENGTH 4

//utility func to parse escaped chars, e.g. 'n' -> '\n'
char getEscapedChar(char ident);

struct CodeStream
{
  CodeStream(string& srcIn, vector<Token*>& toksIn) : src(srcIn), toks(toksIn)
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
    iter++;
    return c;
  }
  char peek(int ahead)
  {
    if(iter + ahead >= src.length())
      return '\0';
    return src[iter + ahead];
  }
  void putback()
  {
    if(iter == 0)
    {
      err("tried to backtrack at start of code stream");
    }
    iter--;
  }
  void addToken(Token* tok)
  {
    toks.push_back(tok);
    toks.back()->line = line;
    toks.back()->col = col;
  }
  //bool value is "eof?"
  operator bool()
  {
    return iter < src.length() && src[iter];
  }
  bool operator!()
  {
    return iter >= src.length() || !src[iter];
  }
  void err(string msg)
  {
    string fullMsg = string("Lexical error at line ") + to_string(line) +
      ", col " + to_string(col) + ": " + msg;
    errAndQuit(fullMsg);
  }
  string& src;
  vector<Token*>& toks;
  size_t iter;
  int line;
  int col;
};

void lex(string& code, vector<Token*>& tokList)
{
  CodeStream cs(code, tokList);
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
      int stringStart = cs.iter;
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
      int stringEnd = cs.iter - 1;
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
      cs.addToken(new StrLit(s));
    }
    else if(c == '/' && cs.peek(0) == '*')
    {
      int commentDepth = 1;
      cs.getNext();
      while(cs && commentDepth)
      {
        //get next char
        char next = cs.getNext();
        if(next == '/' && cs.peek(0) == '*')
        {
          cs.getNext();
          commentDepth++;
        }
        else if(next == '*' && cs.peek(0) == '/')
        {
          cs.getNext();
          commentDepth--;
        }
      }
      //EOF with non-terminated block comment is an error
      if(!cs && commentDepth)
      {
        cs.err("non-terminated block comment (missing */)");
      }
    }
    else if(c == '\'')
    {
      char charVal = cs.getNext();
      if(charVal == '\\')
      {
        cs.addToken(new CharLit(getEscapedChar(cs.getNext())));
      }
      else
      {
        cs.addToken(new CharLit(charVal));
      }
      //finally, expect closing quote
      if(cs.getNext() != '\'')
      {
        cs.err("non-terminated character literal");
      }
    }
    else if(c == '/' && cs.peek(0) == '/')
    {
      cs.getNext();
      while(cs.getNext() != '\n');
    }
    else if(isalpha(c) || c == '_')
    {
      //keyword or identifier
      //scan all following alphanumeric/underscore chars to classify
      int identStart = cs.iter - 1;
      while(true)
      {
        char identChar = cs.getNext();
        if(!isalnum(identChar) && identChar != '_')
        {
          cs.putback();
          break;
        }
      }
      int identEnd = cs.iter;
      string ident = code.substr(identStart, identEnd - identStart);
      //check if keyword
      auto kwIter = keywordMap.find(ident);
      if(kwIter == keywordMap.end())
        cs.addToken(new Ident(ident));
      else
        cs.addToken(new Keyword(kwIter->second));
    }
    else if(c == '0' && tolower(cs.peek(0)) == 'x' && isxdigit(cs.peek(1)))
    {
      //hex int literal, OR int 0 followed by ??? (if not valid hex num)
      cs.getNext();
      cs.getNext();
      char* numEnd;
      unsigned long long val = strtoull(code.c_str(), &numEnd, 16);
      cs.addToken(new IntLit(val));
      for(const char* i = code.c_str() + cs.iter; i != numEnd; i++)
      {
        cs.getNext();
      }
    }
    else if(c == '0' && tolower(cs.peek(0)) == 'b' &&
        (cs.peek(1) == '0' || cs.peek(1) == '1'))
    {
      //binary int literal, OR int 0 followed by ??? (if not valid bin num)
      cs.getNext();
      cs.getNext();
      char* numEnd;
      unsigned long long val = strtoull(code.c_str(), &numEnd, 2);
      cs.addToken(new IntLit(val));
      for(const char* i = code.c_str() + cs.iter; i != numEnd; i++)
      {
        cs.getNext();
      }
    }
    else if(isdigit(c))
    {
      uint64_t intVal = 0;
      //int (hex or dec) or float literal
      //take the integer conversion, or the double conversion if it uses more chars
      const char* numStart = code.c_str() + cs.iter - 1;
      char* intEnd;
      char* floatEnd;
      //note: int/float literals are always positive (- handled as arithmetic operator)
      //this means that IntLit holds unsigned value
      intVal = strtoull(numStart, &intEnd, 10);
      double floatVal = strtod(numStart, &floatEnd);
      if(floatEnd > intEnd)
      {
        //use float
        cs.addToken(new FloatLit(floatVal));
        cs.iter = floatEnd - code.c_str();
        for(const char* i = code.c_str() + cs.iter + 1; i != floatEnd; i++)
        {
          cs.getNext();
        }
      }
      else
      {
        //use int
        cout << "Got decimal int literal: " << intVal << '\n';
        cs.addToken(new IntLit(intVal));
        for(const char* i = code.c_str() + cs.iter; i != intEnd; i++)
        {
          cs.getNext();
        }
      }
    }
    else if(ispunct(c))
    {
      //check for punctuation first (only 1 char)
      auto punctIter = punctMap.find(c);
      if(punctIter == punctMap.end())
      {
        //operator, not punct
        //some operators are 2 chars long, use them if valid, otherwise 1 char
        string oper1 = string("") + c;
        string oper2 = oper1 + cs.peek(1);
        auto oper2Iter = operatorMap.find(oper2);
        if(oper2Iter == operatorMap.end())
        {
          //must be 1-char operator
          auto oper1Iter = operatorMap.find(oper1);
          if(oper1Iter == operatorMap.end())
          {
            cs.err("symbol character neither valid operator nor punctuation.");
          }
          else
          {
            cs.addToken(new Oper(oper1Iter->second));
          }
        }
        else
        {
          cs.addToken(new Oper(oper2Iter->second));
          cs.getNext();
        }
      }
      else
      {
        //c is punct char
        cs.addToken(new Punct(punctIter->second));
      }
    }
    else
    {
      cout << "Note: cs iter is " << cs.iter << ", but code len is " << code.length() << '\n';
      char temp[64];
      sprintf(temp, "unexpected character: 0x%02hhx", c);
      cs.err(temp);
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

