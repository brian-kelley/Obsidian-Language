#ifndef AUTOPTR_H
#define AUTOPTR_H

#include <memory>

#define AP(T) std::shared_ptr<T>

//custom unique ptr
//transfers ownership on assignment/move/copy
/*
template<typename T>
struct AutoPtr
{
  AutoPtr()
  {
    p = nullptr;
    node = nullptr;
  }
  AutoPtr(T* newPtr)
  {
    node = new int(1);
    p = newPtr;
  }
  AutoPtr(const AutoPtr& rhs)
  {
    p = rhs.p;
    node = rhs.node;
    if(node)
      (*node)++;
  }
  AutoPtr(const AutoPtr&& rhs)
  {
    p = rhs.p;
    node = rhs.node;
    //don't need to increment reference count
    (*node)++;
  }
  T& operator*() const
  {
    return *p;
  }
  T* operator->() const
  {
    return p;
  }
  T* operator=(const AutoPtr& rhs)
  {
    if(node)
    {
      (*node)--;
      if(*node == 0)
      {
        delete node;
        delete p;
      }
    }
    p = rhs.p;
    node = rhs.node;
    if(node)
      (*node)++;
    return p;
  }
  operator bool() const
  {
    return p != nullptr;
  }
  bool operator!() const
  {
    return p == nullptr;
  }
  T* get() const
  {
    return p;
  }
  ~AutoPtr()
  {
    if(node)
    {
      (*node)--;
      if(*node == 0)
      {
        //last reference to object has ended, so it can be deleted
        delete node;
        delete p;
      }
    }
  }
  T* p;
  int* node;
};
*/

#endif

