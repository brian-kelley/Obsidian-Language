#ifndef POOLALLOC_H
#define POOLALLOC_H

void* oalloc(size_t bytes);

//Base class that uses extremely fast pool allocator for new
//Only works for things that are never freed (true for parse tree nodes)

//To use it for a struct, just inherit PoolAllocated
struct PoolAllocated
{
  void* operator new(size_t size)
  {
    return oalloc(size);
  }
  void operator delete(void* ptr) {}
};

#endif

