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
    owns = true;
  }
  AutoPtr(T* newPtr)
  {
    p = newPtr;
    owns = true;
  }
  AutoPtr(const AutoPtr& rhs)
  {
    p = rhs.p;
    owns = true;
    rhs.owns = false;
  }
  AutoPtr(const AutoPtr&& rhs)
  {
    p = rhs.p;
    owns = true;
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
    owns = true;
    rhs.owns = false;
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
    if(p && owns)
    {
      delete p;
    }
  }
  mutable T* p;
  mutable bool owns;
};

#endif

