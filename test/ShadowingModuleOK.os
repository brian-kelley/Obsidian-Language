int globalInt = 4;

module InnerModule
{
  int globalInt = 5;
  proc int getGlobalInt()
  {
    return globalInt;
  }
}

proc void main()
{
  assert(globalInt == 4);
  assert(InnerModule.globalInt == 5);
  assert(InnerModule.getGlobalInt() == 5);
}
