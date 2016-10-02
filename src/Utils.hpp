#ifndef UTILS_H
#define UTILS_H

#include "Misc.hpp"

//Read string from file, and append \n
string loadFile(string filename);
//Write string to file
void writeFile(string& text, string filename);

//Print message and exit(EXIT_FAILURE)
void errAndQuit(string message);

#endif

