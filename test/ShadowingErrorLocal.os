A: int = 5;

func problem: int(B: int)
{
  A: int = B + 3;
  return A * 2;
}

proc main: void()
{
  print(problem(3), '\n');
}
