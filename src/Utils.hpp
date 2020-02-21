#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <iostream>

using std::string;
using std::cout;

//Print message and exit(EXIT_FAILURE)
void errAndQuit(string message);

//Read string from file, and append \n
string loadFile(string filename);
//Write string to file
void writeFile(string& text, string filename);

//64-bit encodings in base64
//(to generate compact, somewhat human-readable strings)
string base64Encode(uint64_t num);
uint64_t base64Decode(const string& s);

#endif

