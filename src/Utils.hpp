#ifndef UTILS_H
#define UTILS_H

#include <string>

using std::string;

//Print message and exit(EXIT_FAILURE)
void errAndQuit(string message);

//Read string from file, and append \n
string loadFile(string filename);
//Write string to file
void writeFile(string& text, string filename);

#endif

