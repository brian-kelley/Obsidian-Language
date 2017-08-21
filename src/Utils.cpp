#include "Utils.hpp"

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
  text.resize(size + 2);
  fread((void*) text.c_str(), 1, size, f);
  text[size + 1] = '\n';
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

void errAndQuit(string message)
{
  cout << message << '\n';
  exit(EXIT_FAILURE);
}

bool runCommand(string command)
{
  string silencedCommand = command + " &> /dev/null";
  return system(silencedCommand.c_str()) == 0;
}

/*
#define BLOCK_SIZE 65536
static char* currentBlock = nullptr;
static size_t blockTop;

void* operator new[](std::size_t s) throw(std::bad_alloc)
{
  cout << "Hello from custom allocator!\n";
  if(!currentBlock)
  {
    currentBlock = (char*) malloc(BLOCK_SIZE);
    blockTop = 0;
  }
  //can't fit in a whole block
  if(s > BLOCK_SIZE)
  {
    void* ptr = malloc(s);
    if(!ptr)
      throw std::bad_alloc();
    return ptr;
  }
  if(BLOCK_SIZE - blockTop < s)
  {
    //allocate a new block
    currentBlock = (char*) malloc(BLOCK_SIZE);
    blockTop = 0;
  }
  //return space at end of block
  void* ptr = (void*) (currentBlock + blockTop);
  blockTop += s;
  return ptr;
}

void operator delete[](void* p) throw()
{
  //Do nothing
}
*/

