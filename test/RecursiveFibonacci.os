func int fib(int n)
{
  if(n <= 1)
    return 1;
  return n * fib(n - 1);
}

proc void main()
{
  print(fib(0), '\n');
  print(fib(1), '\n');
  print(fib(3), '\n');
  print(fib(8), '\n');
}

