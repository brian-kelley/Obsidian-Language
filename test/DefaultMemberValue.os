struct A
{
  mem1: int = 5;
  mem2: int;
  mem3: double = 5.3;
}

proc main: void()
{
  a1: A;
  print("All default: ", a1, '\n');
  a2: A = [2, 6, 3.14];
  print("Custom: ", a2, '\n');
}

