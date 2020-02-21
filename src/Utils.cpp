#include "Utils.hpp"
#include <cstdio>
#include <iostream>

void errAndQuit(string message)
{
  std::cerr << message << '\n';
  exit(1);
}

string loadFile(string filename)
{
  FILE* f = fopen(filename.c_str(), "rb");
  if(!f)
  {
    errAndQuit(string("Could not open file \"") + filename + "\" for reading.");
  }
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  rewind(f);
  string text;
  text.resize(size);
  fread((void*) text.c_str(), 1, size, f);
  fclose(f);
  return text;
}

void writeFile(string& text, string filename)
{
  FILE* f = fopen(filename.c_str(), "wb");
  if(!f)
  {
    errAndQuit(string("Could not open file \"") + filename + "\" for writing.");
  }
  fwrite(text.c_str(), 1, text.size(), f);
  fclose(f);
}

//Base 64 encoding:
//A-Z, a-z, 0-9, +, /
//
//Takes 6 bits at a time, from LSB to MSB (until 0)
//This way, values use as few characters as possible

string base64Encode(uint64_t num)
{
  string s;
  while(num)
  {
    int bits = num % 64;
    num /= 64;
    if(bits < 26)
    {
      s += ('A' + bits);
      continue;
    }
    bits -= 26;
    if(bits < 26)
    {
      s += ('a' + bits);
      continue;
    }
    bits -= 26;
    if(bits < 10)
    {
      s += ('0' + bits);
      continue;
    }
    bits -= 10;
    if(bits == 0)
      s += '+';
    else
    {
      s += '/';
    }
  }
  return s;
}

uint64_t base64Decode(const string& s)
{
  uint64_t val = 0;
  for(int i = s.length() - 1; i >= 0; i--)
  {
    val *= 64;
    char c = s[i];
    if(c >= 'A' && c <= 'Z')
      val += (c - 'A');
    else if(c >= 'a' && c <= 'z')
      val += 26 + (c - 'a');
    else if(c >= '0' && c <= '9')
      val += 52 + (c - '0');
    else if(c == '+')
      val += 62;
    else if(c == '/')
      val += 63;
    else
      exit(2);
  }
  return val;
}

