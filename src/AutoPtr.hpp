#ifndef AUTOPTR_H
#define AUTOPTR_H

#define AP(T) AutoPtr<T>

//custom unique ptr
//transfers ownership on assignment/move/copy
template<typename T>
struct AutoPtr
{
  AutoPtr()
  {
    p = nullptr;
  }
  AutoPtr(T* newPtr)
  {
    p = newPtr;
  }
  AutoPtr(const AutoPtr& rhs)
  {
    p = rhs.p;
    //rhs.p = nullptr;
  }
  AutoPtr(const AutoPtr&& rhs)
  {
    p = rhs.p;
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
    p = rhs.p;
    //rhs.p = nullptr;
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
  ~AutoPtr()
  {
    if(p)
    {
      //delete p;
    }
  }
  mutable T* p;
};

#endif

