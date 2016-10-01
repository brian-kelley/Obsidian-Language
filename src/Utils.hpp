#ifndef UTILS_H
#define UTILS_H

#include "Misc.hpp"

//Allocate a buffer and load a file into it
//Always append one newline and a \0 at the end
string loadFile(string filename);
void writeFile(string& text, string filename);

void errAndQuit(string message);

#endif
