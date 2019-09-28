#ifndef COMMON
#define COMMON

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <stack>
#include <queue>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <ctime>
#include <climits>
#include <cassert>
#include <stdexcept>
#include <new>
#include <utility>
#include <tuple>
#include "variant.h"

using std::string;
using std::vector;
using std::map;
using std::unordered_map;
using std::unordered_set;
using std::set;
using std::stack;
using std::queue;
using std::pair;
using std::tuple;
using std::ostream;
using std::ofstream;
using std::ostringstream;
using std::exception;
using std::runtime_error;
using std::cout;
using std::endl;
using std::to_string;

//Interpreter output capture:
//this either points to cout or capturedOutput (ostringstream)
extern ostream& interpreterOut;

string getInterpreterOutput();

typedef ostringstream Oss;

//None type useful for working with variants
struct None{};

struct Node;

//Whether compiler is in debug mode (enabled = diagnostic output)
#define DEBUG

//Read string from file, and append \n
string loadFile(string filename);
//Write string to file
void writeFile(string& text, string filename);

//Print message and exit(EXIT_FAILURE)
void errAndQuit(string message);

//Run a command (return true if success, and return the elapsed time)
//if silenced, suppress all output to stdout and stderr
bool runCommand(string command, bool silenced = false);

string getSourceName(int id);

#define errMsg(msg) {ostringstream oss_; oss_ << msg; errAndQuit(oss_.str());}

#define errMsgLocManual(fileID, line, col, msg) \
{ostringstream oss_; oss_ << "Error in " << getSourceName(fileID) << ", " << line << "." << col << ":\n" << msg; errAndQuit(oss_.str());}

#define errMsgLoc(node, msg) errMsgLocManual(node->fileID, node->line, node->col, msg)

#define warnMsgLocManual(fileID, line, col, msg) \
{ostringstream oss_; oss_ << "Warning: " << getSourceName(fileID) << ", " << line << "." << col << ":\n" << msg; cout << (oss_.str());}

#define warnMsgLoc(node, msg) warnMsgLocManual(node->fileID, node->line, node->col, msg)

#define IE_IMPL(f, l) {cout << "<!> Onyx internal error: " << f << ", line " << l << '\n'; int* asdf = nullptr; asdf[0] = 4; exit(1);}

#define INTERNAL_ERROR IE_IMPL(__FILE__, __LINE__)
#define INTERNAL_ASSERT(cond) {if(!(cond)) {INTERNAL_ERROR}}
//Debug macros:
//DEBUG_DO does something only when DEBUG is defined
//TIMEIT(name, stmt) does stmt, but times it and prints the time if DEBUG

#ifdef DEBUG
#define DEBUG_DO(x) x
#else
#define DEBUG_DO(x)
#endif

#ifdef DEBUG
#define TIMEIT(name, stmt) \
{ \
  auto _startClock = clock(); \
  stmt; \
  std::cout << name << " took " << ((double) (clock() - _startClock)) / CLOCKS_PER_SEC << " sec.\n"; \
}
#else
#define TIMEIT(name, stmt) stmt
#endif

//generate a character for use in C code (i.e. "a" or "\n" or "\x4A")
//doesn't add any quotes
string generateChar(char ch);
//generate a character for use in dotfile, which processes one level of escapes
string generateCharDotfile(char ch);

template<typename T>
vector<T> operator+(const vector<T>& lhs, const vector<T>& rhs)
{
  vector<T> cat;
  cat.reserve(lhs.size() + rhs.size());
  cat.insert(lhs.begin(), lhs.end());
  cat.insert(rhs.begin(), rhs.end());
  return cat;
}
template<typename T>
void operator+=(vector<T>& lhs, const vector<T>& rhs)
{
  lhs.reserve(lhs.size() + rhs.size());
  lhs.insert(rhs.begin(), rhs.end());
}

//FNV-1a hash (for use with unordered map and set)
//https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
struct FNV1A
{
  FNV1A()
  {
    state = 14695981039346656037ULL;
  }
  //Hash a single object
  template<typename T>
  void pump(const T& data)
  {
    auto bytes = (unsigned char*) &data;
    for(size_t i = 0; i < sizeof(T); i++)
    {
      pumpByte(bytes[i]);
    }
  }
  //Hash an array
  template<typename T>
  void pump(const T* data, size_t n)
  {
    auto bytes = (unsigned char*) data;
    for(size_t i = 0; i < n * sizeof(T); i++)
    {
      pumpByte(bytes[i]);
    }
  }
  size_t get()
  {
    return state;
  }
  private:
  void pumpByte(unsigned char b)
  {
    state = (state ^ b) * 1099511628211ULL;
  }
  size_t state;
};

template<typename T>
size_t fnv1a(T&& data)
{
  FNV1A f;
  f.pump(data);
  return f.get();
}

//FNV-1a for pointer
template<typename T>
size_t fnv1a(T* ptr)
{
  FNV1A f;
  f.pump<T*>(ptr);
  return f.get();
}

//FNV-1a for contiguous arrays
template<typename T>
size_t fnv1a(T* data, size_t n)
{
  FNV1A f;
  f.pump(data, n);
  return f.get();
}

template<typename T>
void quickRemove(vector<T>& vec, size_t i)
{
  if(i != vec.size() - 1)
  {
    //overwrite the deleted element
    vec[i] = vec[vec.size() - 1];
  }
  vec.pop_back();
}

template<typename T, typename F>
void removeIf(vector<T>& vec, F f)
{
  std::remove_if(vec.begin(), vec.end(), f);
}

#endif

