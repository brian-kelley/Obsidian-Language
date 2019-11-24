func fib: int(int n)
{
  if(n <= 1)
    return n;
  return fib(n - 2) + fib(n - 1);
}

proc main: void()
{
  for i : 0,13
  {
    print("f(", i, ") : ", fib(i), '\n');
  }
}

