#ifndef COMMON
#define COMMON

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <ctime>
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <new>

using std::string;
using std::vector;
using std::map;
using std::set;
using std::ostream;
using std::ofstream;
using std::ostringstream;
using std::exception;
using std::runtime_error;
using std::cout;
using std::endl;
using std::to_string;

typedef ostringstream Oss;

//Whether compiler is in debug mode (enabled = diagnostic output)
//#define DEBUG

//Read string from file, and append \n
string loadFile(string filename);
//Write string to file
void writeFile(string& text, string filename);

//Print message and exit(EXIT_FAILURE)
void errAndQuit(string message);

//Run a command (return true if success, and return the elapsed time)
//suppress all output to stdout and stderr
bool runCommand(string command);

#define ERR_MSG(msg) {ostringstream oss_; oss_ << msg; errAndQuit(oss_.str());}
#define ERR_MSG_LOC(msg) {ostringstream oss_; oss_ << msg; errAndQuit(oss_.str());}

#define IE_IMPL(f, l) {cout << "<!> Onyx internal error: " << f << ", line " << l << '\n'; exit(1);}

#define INTERNAL_ERROR IE_IMPL(__FILE__, __LINE__)
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

#endif

