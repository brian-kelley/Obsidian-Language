#include "PoolAlloc.hpp"

unsigned char* block = NULL;
size_t top = 0;

void* oalloc(size_t bytes)
{
  /*
  if(bytes > 1024)
  {
    return malloc(bytes);
  }
  if(block == NULL)
    block = (unsigned char*) malloc(65536);
  if(65536 - bytes > top)
  {
    block = (unsigned char*) malloc(65536);
    top = 0;
  }
  unsigned char* ptr = block + top;
  top += bytes;
  return ptr;
  */
  return malloc(bytes);
}

