#ifndef UTILS_H
#define UTILS_H

#include "Misc.hpp"
#include <sstream>

//Read string from file, and append \n
string loadFile(string filename);
//Write string to file
void writeFile(string& text, string filename);

//Print message and exit(EXIT_FAILURE)
void errAndQuit(string message);

#define IE_IMPL(f, l) cout << "<!> Onyx internal error: " << f << ", line " << l << '\n'

#define INTERNAL_ERROR IE_IMPL(__FILE__, __LINE__)

#endif

