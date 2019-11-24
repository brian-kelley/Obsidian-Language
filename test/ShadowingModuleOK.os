int globalInt = 4;

module InnerModule
{
  int globalInt = 5;
  proc getGlobalInt: int()
  {
    return globalInt;
  }
}

proc main: void()
{
  assert(globalInt == 4);
  assert(InnerModule.globalInt == 5);
  assert(InnerModule.getGlobalInt() == 5);
}
