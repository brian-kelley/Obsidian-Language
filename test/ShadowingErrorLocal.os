int A = 5;

func problem: int(int B)
{
  int A = B + 3;
  return A * 2;
}

proc main: void()
{
  print(problem(3), '\n');
}
